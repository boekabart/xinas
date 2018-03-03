// Xinas.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#define BAT_RAD 1.0f
#define BAL_RAD 0.5f
#define BAT_VEL 7.5f
#define BAL_MASS 1.0f
#define BAL_FRIC 1.0f
#define BAL_IMASS 0.2f
#define MINGOALDT 0.5f
#define ACCEL	5.0f

#define MAX_VEL 25.0f
#define MIN_VEL 5.0f

//#pragma comment (lib,"Direct3D8S.lib")
#pragma comment (lib,"citkCore.lib")
#pragma comment (lib,"citkIO.lib")
#pragma comment (lib,"RT_gl.lib")
#pragma comment (lib,"glu32.lib")
#pragma comment (lib,"Opengl32.lib")

//-----------------------------------------------------------------------------
// Name: main()
// Desc: The application's entry point
//-----------------------------------------------------------------------------

int MapPos[] = { 'b' , ']' , 'n' , 'q' };
int MapNeg[] = { 'v' , '=' , 'm' , 'a' };

// Analog
enum {
	LeftX = 0, LeftY, RightX, RightY,
	A, B, X, Y,
	Black, White, LeftTrigger, RightTrigger,
	LeftMotor, RightMotor, 
	ACount
};

// Digital
enum {
	Plugged = 0,
	Up, Down, Left, Right,
	Start, Back, LeftThumb, RightThumb,
	DCount
};

int BtnPos[] = { Right, Down, Left, Up };
int BtnNeg[] = { Left, Up, Right, Down };

int Axis[] = { LeftX, LeftY, LeftX, LeftY };
float mAxis[] = { 1.0, -1.0, -1.0, 1.0 };

DColor Colors[] = { COL_CYAN, COL_YELLOW, COL_PURPLE, COL_RED, COL_GREEN, COL_BLUE };

struct DPlayer;

struct DSinas
{
	Vector			Vel;
	Vector			Pos;
	Vector			AngVel;

	float			Rad;

	DSinas();
	~DSinas() { Sini.Remove(this); if (Viz) Viz->RemoveFromWorld(); }

	DPlayer*		Owner;

	PEntity			Viz;

	static ArrayPtr<DSinas> Sini;
};

struct DPlayer
{
	void			Scored();
	void			Batted();
	bool			Missed(); // true if it counts

	DynString		Name;
	enum		playmode_t
		{
			pmDigital=0, pmDirect, pmAccel, PMCOUNT
		};

	struct IO
	{
		StrongPtr<CTHardwareIO>	hwio; // NOT pHardwareIO!!! it's shared!
		int			nPosBtn;
		int			nNegBtn;

		int			nPosAxis;
		float		aPosDir;
		float		aPosNull;
		int			nNegAxis;
		float		aNegDir;
		float		aNegNull;

		int			nAxis;
		float		mAxis; //multiplier (-1 or 1 typ.)

		DSmartBool	Pos;
		DSmartBool	Neg;

		void		Update();

		bool		autoPilot;
		playmode_t	PlayMode;

		DSmartBool	ModeButton;

		IO() : autoPilot(true), nPosAxis(-1), nNegAxis(-1), nPosBtn(-1), nNegBtn(-1), PlayMode(pmDirect) {}
	} io;

	Plane		Goal;
	float		GoalHWidth;

	float		MissedTimer;

	float		SmallRumble;
	float		BigRumble;

	PEntity		Viz; // goal
	PGroup		Scores; // emeralds
	PGroup		MinScores; // emeralds
	PGroup		Games; // emeralds
    
	DSinas		Bat;

	float		Pos; // 
	float		Dir; // -1 , 0 or 1 normally

	void		UpdatePos( float dt );

	int			Score, MinScore;

	int			nGames;

	DPlayer();
};

struct DBall : public DSinas
{
	DPlayer*		Player;
	DPlayer*		PrevPlayer;

	float			Grav, Grav2;

	void			UpdateBall( float dt, ArrayPtr<DPlayer>& players );

	DBall();
};

class DGame
{
public:
	DGame() : Quit(false), Waiting(true),DoCamThing(true),Winner(-1),LastWinner(-1) {}
	~DGame() { pl.DeleteAll(); bl.DeleteAll(); }

	int		Winner, LastWinner;

	float	CountDown;

	ArrayVal<float> timers;

	ArrayPtr<DPlayer> pl;
	ArrayPtr<DBall> bl;
	
	PGroup	root;
	PGroup	field;
	PGroup	bats;
	PGroup	balls;
	PGroup	counter;

	PBasicSpectator spec;

	void	CreatePlayers( int n );
	void	CreateBalls( int n );
	void	CreateScene();

	// order per frame
	void	Interface( float dt );
	void	UpdateGame( float dt );
	void	UpdateScene( float dt );

	bool	Quit;
	bool	DoCamThing;
	bool	Waiting;
};

void DPlayer::Scored()
{
	Score++;
	//todo: rumble in the bronx
	SmallRumble = 1.5;
}

bool DPlayer::Missed()
{
	if (MissedTimer<MINGOALDT)
		return false;
	MinScore++;
	BigRumble = 1.5;
	MissedTimer = 0.0f;
	return true;
	//todo: rumble in the bronx
}

void DPlayer::Batted()
{
	SmallRumble = 0.6f;
	//todo: rumble in the bronx
}

inline bool GetA( CTHardwareIO* io, int na, float dir, float nul )
{
	if (na<0)
		return false;
	float v = io->GetA(na);
	if (fabs(nul)<0.335) // (-1 to ) 0 to 1: thresh. is at dir*0.5;
		return dir<0?(v<-0.5):(v>0.5);
	// 0 to 0.5 to 1: thresh at 0.33 and .66
	return dir<0?(v<0.335):(v>0.665);
}

inline bool GetD( CTHardwareIO* io, int nd )
{
	if (!_smaller(nd, io->GetDCount()))
		return false;
	return io->GetD(nd);
}

void DPlayer::IO::Update()
{
	if (!hwio)
		return;

	hwio->Refresh();

	if (autoPilot || !hwio->GetD(Plugged)) // ap!
		return;

	Pos = (GetD(hwio, nPosBtn) || GetA(hwio,nPosAxis,aPosDir,aPosNull));
	Neg = (GetD(hwio, nNegBtn) || GetA(hwio,nNegAxis,aNegDir,aNegNull));
	ModeButton = (hwio->GetA(Y)>0.3);
}

void DPlayer::UpdatePos( float dt )
{
	MissedTimer += dt;

	BigRumble -= _min(BigRumble,5.0f * dt);
	SmallRumble -= _min(SmallRumble,5.0f * dt);

	if (!_feqz(dt))	// not in paused mode
	{
   		if (io.hwio)
		{
			io.hwio->SetA(LeftMotor, _min(BigRumble,1.0f));
			io.hwio->SetA(RightMotor, _min(SmallRumble,1.0f));
		}

		io.Update();
	}

	if (io.ModeButton.ToggledOn())
	{
		(*(int*)&io.PlayMode)++;
		if (io.PlayMode>=PMCOUNT)
			io.PlayMode=pmDigital;
	}

	bool direct = (io.nAxis>=0) && io.PlayMode==pmDirect &&!io.autoPilot && io.hwio && io.hwio->GetD(Plugged);
	bool accel = (io.nAxis>=0) && io.PlayMode==pmAccel &&!io.autoPilot && io.hwio && io.hwio->GetD(Plugged);
	bool digital = !direct && !accel;
	if (direct)
		Dir = 1.0 * io.hwio->GetA(io.nAxis)*io.mAxis;
	else if ( accel )
		Dir = _limit( 0.999*Dir + dt * ACCEL * io.hwio->GetA(io.nAxis)*io.mAxis, -1.0, 1.0);
	else // digital
	{
		if (io.Pos.ToggledOn())
			Dir = 1.0;
		if (io.Neg.ToggledOn())
			Dir = -1.0;
	}

	int oPos = Pos;
	Pos += BAT_VEL*Dir*dt;
	if (Pos>GoalHWidth)
	{
		Pos = GoalHWidth;//(GoalHWidth*2) - Pos;
		Dir = direct?0.0:-fabs(Dir);
	}
	else if (Pos<-GoalHWidth)
	{
		Pos = -GoalHWidth;//(GoalHWidth*-2) - Pos;
		Dir = direct?0.0:fabs(Dir);
	}

	Vector traveldir = Goal.normal%Vector(0,1,0);
	Bat.Pos = Goal.normal*Goal.dist + traveldir*Pos;

	for (int q=0; q<DSinas::Sini.Count(); q++)
	{
		DSinas* s = DSinas::Sini[q];
		if (!s->Owner)
			continue;
		if (s->Owner==this)
			continue;
		Vector dif = s->Pos - Bat.Pos;
		float d = dif.Length();
		if (d<(Bat.Rad+s->Rad))
		{
			if (Pos>0)
				Dir = direct?0.0:-fabs(Dir);
			else
				Dir = direct?0.0:fabs(Dir);

			Pos = oPos;
			Bat.Pos = Goal.normal*Goal.dist + traveldir*Pos;
			break;
		}
	}

	Bat.Vel = Dir * BAT_VEL * traveldir;
}

ArrayPtr<DSinas> DSinas::Sini;

DSinas::DSinas()
{
	Rad = 1.0;
	Pos.Zero();
	Vel.Zero();

	Owner = NULL;
	Sini.Add(this);
}

DBall::DBall()
{
	Grav2 = 0.0;
	Grav = -3.0;

	Pos.Set( -0.5 + rand()/(float)RAND_MAX, 0, -0.5 + rand()/(float)RAND_MAX );
	Vel.Set( -0.5 + rand()/(float)RAND_MAX, 0, -0.5 + rand()/(float)RAND_MAX );
	AngVel.Zero();
	Pos *= 3;
	Vel *= 3;

	Player = PrevPlayer = NULL;
}

DPlayer::DPlayer()
{
	MissedTimer = MINGOALDT;
	Pos = Dir = GoalHWidth = 0;
	Bat.Owner = this;
	Score = MinScore = 0;
	SmallRumble = BigRumble = 0;
	nGames = 0;
}

void DBall::UpdateBall( float dt, ArrayPtr<DPlayer>& pl ) // pl for goals only
{
	if (!_feqz(Grav)) // Linear grav field
	{
		float l = Pos.Length();
		Vector nPos = Pos/l;
		_set_max(l, 0.1f);
		Vector force = nPos * Grav / l;
		Vel -= force * dt;
	}
	if (!_feqz(Grav2)) // Quadric grav field
	{
		float l = Pos.Length();
		Vector nPos = Pos/l;
		_set_max(l, 0.1f);
		Vector force = nPos * Grav2 / l*l;
		Vel -= force * dt;
	}

	//Vel.y -= 9.81 * dt;

	//if (0)
	{
		Vector surfacevel;
		surfacevel.x = -Rad * AngVel.z;
		surfacevel.y = 0;
		surfacevel.z = Rad * AngVel.x;
		Vector svd = surfacevel - Vel;
		Vector fors = BAL_FRIC * dt * svd;
		Vel += fors / BAL_MASS;
		AngVel.x -= fors.z / BAL_IMASS;
		AngVel.z += fors.x / BAL_IMASS;
	}

	float vl = Vel.Length();
	if (vl>MAX_VEL)
		Vel *= MAX_VEL/vl;
	if (vl<MIN_VEL)
		Vel *= MIN_VEL/vl;

	Pos += dt * Vel;

	if ( false && Pos.y<0)
	{
		Pos.y=0;

		float imp = -1.9 * Vel.y * BAL_MASS;
		float nvy = -0.9 * Vel.y;
		Vel.y = 0;

		Vector surfacevel;
		surfacevel.x = -Rad * AngVel.z;
		surfacevel.y = 0;
		surfacevel.z = Rad * AngVel.x;
		Vector svd = surfacevel - Vel;
		Vector fors = imp * dt * svd;
		Vel += fors / BAL_MASS;
		AngVel.x -= fors.z / BAL_IMASS;
		AngVel.z += fors.x / BAL_IMASS;

		Vel.y = 0;//nvy;
	}

	for (int q=0; q<Sini.Count(); q++)
	{
		DSinas& s = *Sini[q];
		if (&s==this)
			continue;
		// Check coll with ball
		Vector dif = Pos - s.Pos;
		float d = dif.Length();
		float intrusion = (Rad+s.Rad) - d;
		if (intrusion>=0) //collision
		{
			Vector coldir = dif/d;
			float colspeed = (Vel - s.Vel) ^ coldir;
			if (colspeed>0)
				continue; // outgoing already
			if (s.Owner) // player's bat
			{
				// Take the whole impulse
				Vel -= (2.0*colspeed) * coldir; // That's the impulse, 10% less

			// see if the ball
				if (Player != s.Owner)
				{
					PrevPlayer = Player;
					Player = s.Owner;
					Player->Batted();
				}
			}
			else //(other ball?)
			{
				Vel -= 1.1*colspeed * coldir;
				s.Vel += 1.1*colspeed * coldir;
			}
		}
	}
	for (int q=0; q<pl.Count(); q++)
	{
		DPlayer& p = *pl[q];

		// Check coll with goal
		float d = -p.Goal.Distance( Pos );
		if (d<Rad)
		{
			// Goal

			float colspeed = Vel^p.Goal.normal;
			if (colspeed>=0) //outgoing already
				continue;
			Vel -= (2.0*colspeed) * p.Goal.normal;

			// Points for Player
			if (!Player && !PrevPlayer)
				continue; // lucky someone!

			if (!p.Missed())
				continue; // no count!

			if (Player && (Player!=&p))
				Player->Scored();
			else if (PrevPlayer)
				PrevPlayer->Scored();
		}
	}
}

bool quit = false;
#ifndef _XBOX
# include <conio.h>
#endif


void DGame::CreatePlayers( int n )
{
	float rad = BAT_RAD;

	while (pl.Count()>n)
		delete pl.Pop();

	while (pl.Count()<n)
		pl.Add(new DPlayer);

	while (timers.Count()<n)
		timers.Add(1);

	while (timers.Count()>n)
		timers.Pop();

	float r = 10;

	float dh = M_2PI / n;
	float h = -0.5 * dh;
	for (int q=0; q<n; q++)
	{
		DPlayer& p = *pl[q];

		p.io.nNegBtn = BtnNeg[q];
		p.io.nPosBtn = BtnPos[q];
		p.io.nAxis = Axis[q];
		p.io.mAxis = mAxis[q];

		p.Bat.Viz.Release();

		Vector v1(sin(h)*r, 0, cos(h)*r);
		h+=dh;
		Vector v2(sin(h)*r, 0, cos(h)*r);
		Vector avg = 0.5 * (v1+v2);
		p.GoalHWidth = (v1-avg).Length();

		p.Pos = 0;
		p.Bat.Pos = avg;
		p.Bat.Rad = rad;
		p.Bat.Vel.Zero();
		p.Dir = 0.0;

		float al = avg.Length();
		p.Goal.normal = avg/-al;
		p.Goal.dist = -al;
	}
}

void DGame::CreateBalls( int n )
{
	while (bl.Count()>n)
		delete bl.Pop();

	while (bl.Count()<n)
	{
		bl.Add(new DBall);
		bl.Peek()->Rad = BAL_RAD;
	}
}

void DGame::CreateScene()
{
	int q;
/*
	for (q=0; q<DSinas::Sini.Count(); q++)
		DSinas::Sini[q]->Viz.Release();
	root.Release();
	spec.Release();
*/

	if (!root)
	{
		root.New();
		root->Name = "Root";

		PGroup camroot;
		camroot.New();
		camroot->Name = "CamRoot";
		camroot->SetParent(root);

		PCamera cam;
		cam.New();
		cam->SetParent(camroot);
		cam->MoveToY(20);
		cam->MoveToZ(-5);
		cam->SetDirection(Vector(0,-2,0)-cam->GetPosition());
		cam->SetFOV(50);

		spec.New();
		spec->Viewport.SetBackgroundColor(COL_RED*0.5f);
		spec->UseVideo(true);
		spec->UseAudio(false);
		spec->RegisterWithService();
		spec->SetCamera(cam);
		spec->Viewport.SetVerCameraRect(-1,1);
//		spec->SetVideoDevice( (CTRenderTarget*)citk_ObtainDeviceByName("CTRenderTarget_GL"));
		spec->Name = "Spectator";
		spec->Enable();
		citk_ShowPropertyPage(spec);

		P3DText txt;
		txt.New();
		txt->Load(_T("shadowfont128"));
        txt->UseLocalSpace();
		txt->MoveToY(4);
		txt->MoveToZ(5);
		txt->SetText("Hallo");
		txt->SetParent(root);
		txt->Name = "text";
		PMotionController mmm;
		mmm.New();
		mmm->MotionInfo.AngVel.x = 3.0;
		mmm->SetObject(txt);
		mmm->Enable();
		mmm->Name = "TextMotionController";

		PMaterialProperty mp;
		mp.New();
		mp->SetColor( COL_WHITE*0.3f );
		mp->SetSpecular( COL_WHITE*1.0f );
		mp->Power = 50;

		float dik = 0.3f;
		float rad = 5 * sqrt(2.0f);
		int n = 10;
		float d = 2*rad/(float)n;

		balls.New();
		balls->SetParent(root);
		balls->Name = "Balls";

		bats.New();
		bats->SetParent(root);
		bats->Name = "Batjes";

		field.New();
		field->SetParent(root);
		field->Name = "Field";

		PGroup g2;
		g2.New();
		g2->SetParent(field);
		g2->MoveToY(-10);
		g2->ScaleBy(4);
		g2->CreateRenderState()->SetRenderProperty( mp );
		g2->Name = "MotionField";

		PMotionController mc;
		mc.New();
		mc->MotionInfo.AngVel.y = 0.3f;
		mc->SetObject(g2);
		mc->Enable();
		mc->Name = "FieldMotionController";

		PGroup g;
		g.New();
		g->SetParent(field);
		g->MoveToY(-BAL_RAD - 0.5 * dik);
		g->CreateRenderState()->SetRenderProperty( mp );
		g->Name = "StaticField";
		PBox b;
		b.New();
		b->SetDimensions( Vector( 0.95*d, dik, 0.95*d ) );
		for (int x=0; x<n; x++)
		{
			float rx = -rad + 0.5 * d + x*d;
			for (int z=0; z<n; z++)
			{
				float rz = -rad + 0.5 * d + z*d;
				
				PDrawable d;
				d.New();
				d->MoveToX(rx);
				d->MoveToZ(rz);
				d->SetShape(b);
				d->SetParent(g);

				d.New();
				d->MoveToX(rx);
				d->MoveToZ(rz);
				d->SetShape(b);
				d->SetParent(g2);
			}
		}
		root->CreateBoundingVolume(CTDrawable::kind_t::DRAW);
	}

	for ( q=0; q<DSinas::Sini.Count(); q++)
	{
		DSinas* s = DSinas::Sini[q];
		if (!s->Viz)
		{
			PMaterialProperty mp;
			mp.New();
			mp->SetColor(COL_WHITE);

			PGroup g;
			g.New();
			g->SetParent(s->Owner?bats:balls);
			g->CreateRenderState()->SetRenderProperty(mp);
			g->Name = s->Owner?"BatjeGroup":"SinasGroup";
			s->Viz = g;

			PSphere sp;
			sp.New();

			PDrawable d;
			d.New();
			d->SetShape(sp);
			d->SetParent(g);
			d->Name = "Bal";

			PBox bx;
			bx.New();
			bx->SetDimensions(0.5);

			d.New();
			d->SetShape(bx);
			d->SetParent(g);
			d->Name = "SpinBox";

			PMotionController mc;
			mc.New();
			mc->MotionInfo.AngVel.Set(rand()/(float)RAND_MAX,rand()/(float)RAND_MAX,rand()/(float)RAND_MAX);
			mc->MotionInfo.AngVel*=5.0;
			mc->SetObject(d);
			mc->Enable();

			if (!s->Owner)
			{
				mc.New();
				mc->SetObject(g);
				mc->Enable();
			}
		}

		s->Viz->MoveTo( s->Pos );

		DTreeClassIterator<CTDrawable> id(s->Viz);
		PDrawable d;
		d.DynamicCast( id.Found(0) );
		if (!d)
			continue; // cannot update rad;

		PSphere sp;
		sp.DynamicCast( d->GetShape() );
		if (sp && !_feq( s->Rad, sp->GetRadius() ))
			sp->SetRadius( s->Rad );

		// Relight my fire
		if (s->Owner) //bat
			continue;

		DBall* bal = (DBall*)s;
		if (bal->Viz->RenderState)
			bal->Viz->RenderState->RemoveRenderProperty(CD(CTLightingProperty));
		if (bal->Player)
		{
			DTreeClassIterator<CTLight> i(bal->Player->Viz);
			if (i.Found(0))
				i.Found[0]->IncludeObject(bal->Viz);
		}
	}

	for ( q=0; q<pl.Count(); q++)
	{
		DPlayer* p = pl[q];
		if (p->Viz)
			continue;
		PMaterialProperty mp;
		mp.New();
		mp->SetColor( COL_WHITE );
		mp->SetSpecular( COL_WHITE*0.6f );
		mp->Power = 50;

		PGroup g;
		g.New();
		g->MoveTo(p->Goal.normal * p->Goal.dist);
		g->SetDirection(p->Goal.normal);
		pl[q]->Bat.Viz->SetAz(g->GetAz()+90 DEGREES);
		g->CreateRenderState()->SetRenderProperty(mp);
		g->Name = "GoalGroup";

		Vector along = p->Goal.normal%Vector(0,1,0);

		int n = 7;
		int hn = n/2;
		float fatness = 1.1f * BAT_RAD;
		float height = 2.2f * BAT_RAD;

		g->MoveBack( 0.5 * fatness );

		PBox b;
		b.New();
		b->SetDimensions(Vector( p->GoalHWidth * 2.0f/(float)n, height, fatness ) );

		for (int w=-hn; w<=hn; w++)
		{
			PDrawable d;
			d.New();
			d->MoveToX( p->GoalHWidth * 2.0f*w/(float)n );
			d->SetShape(b);
			d->SetParent(g);
			d->Name = "Goal";
		}

		PGroup x;
		x.New();
		x->Name = "MinScores";
		x->MoveToX(-p->GoalHWidth);
		x->MoveToY( 0.25 + 0.5 * height );
		x->SetParent( g );
		p->MinScores = x;

		x.New();
		x->Name = "Scores";
		x->MoveToX(-p->GoalHWidth);
		x->MoveToY( 0.25 + 0.5 * height );
		x->SetParent( g );
		p->Scores = x;

		x.New();
		x->Name = "Games";
		x->MoveToY( 0.75 + 1.5 * height );
		x->SetParent( g );
		p->Games = x;

		PLight l;
		l.New();
		l->SetColor(0.3f*Colors[q]);
		l->MoveToY( 4 );
		l->MoveToZ (2.0 + fatness);
		l->SetType(CTLight::POINT);
		l->SetRangeInvSquare( p->GoalHWidth * 15.0f );
//		l->CalcRange( 1,2,3);
		l->SetParent(g);

		p->Viz = g;
		g->SetParent(root);
	}

	// Light Includes
	for ( q=0; q<pl.Count(); q++)
	{
		DPlayer* p = pl[q];
		if (!p->Viz)
			continue;
		DTreeClassIterator<CTLight> it(p->Viz);
		CTLight* l = it.Found(0);
		if (!l)
			continue;

		l->IncludeObject( p->Viz ); // Own Goal
		l->IncludeObject( field ); // Whole Field
		l->IncludeObject( bats ); // All Bats
		for (int w=0; w<pl.Count(); w++)
		{
			// And all emeralds
			l->IncludeObject( pl[w]->Scores );
			l->IncludeObject( pl[w]->MinScores );
			l->IncludeObject( pl[w]->Games );
		}
	}
}

void DGame::UpdateScene( float dt )
{
	if (!spec)
		return;
	CreateScene();
	static float t = 0.0f;
	t+=dt;
	float s = sin(t);

	if (!Waiting && CountDown>0)
	{
		if (!counter)
		{
			PMaterialProperty mp;
			mp.New();
			mp->SetColor(COL_WHITE);

			counter.New();
			counter->SetParent(field);
			counter->CreateRenderState()->SetRenderProperty(mp);
		}
		else
			counter->Show();

		int num = CountDown;
		num++; // eg. 2.1 sec: 3 blocks
		if (counter->GetChildCount()<num)
		{
			float hsz = 9 * sqrt(0.5);
			float hboxsize = hsz/(float)num;

			counter->DeleteChildren();
			PBox bx;
			bx.New();
			bx->SetDimensions(Vector(0.95f*2.f*hboxsize, 1.0, 1.f*hsz));

			int q;
			for (q=0; q<num; q++)
			{
				PDrawable d;
				d.New();
				d->MoveTo(Vector(-hsz + (2*q+1)*hboxsize,1.0,0 ));
				d->SetShape(bx);
				d->SetParent( counter );
			}
		}
		else
		{
			int q;
			for (q=0; q<num; q++)
				counter->GetChildNo(q)->Show();
			for (;q<counter->GetChildCount(); q++)
				counter->GetChildNo(q)->Hide();
		}
	}
	else if (counter)
		counter->Hide();

	int q;
	for ( q=0; q<pl.Count(); q++)
	{
		DPlayer* p = pl[q];
		if (!p->Viz)
			continue;
		DTreeClassIterator<CTLight> il(p->Viz);
		if (!il.Found(0))
			continue;
		il.Found[0]->MoveToX(s*p->GoalHWidth);

		int max = (2.0f/0.6f)*p->GoalHWidth;
#if 0
		int toomuch = _max3(0,p->MinScore - max, p->Score-max);

		int MinScore = _max(0,p->MinScore-toomuch);
		int Score = _max(0,p->Score-toomuch);
#else
		int MinScore =_limit(p->MinScore-p->Score,0,max);
		int Score = _limit(p->Score-p->MinScore,0,max);
		if (LastWinner<0 && Winner<0 && Score==max)
			Winner = q;
			
#endif

		CTGroup* ms = p->MinScores;
		int w;
		for (w=0; w<MinScore&&w<ms->GetChildCount(); w++)
			ms->GetChildNo(w)->Show();
		for (w=MinScore; w<ms->GetChildCount(); w++)
			ms->GetChildNo(w)->Hide();
		while (ms->GetChildCount()<MinScore)
		{
			PSphere sp;
			sp.New();
			sp->SetRadius(0.25);
			PDrawable d;
			d.New();
			d->SetShape( sp );
			d->MoveToX( 0.3 + 0.6 * ms->GetChildCount() );
			d->SetParent( ms );
		}

		CTGroup* sc = p->Scores;
		for (w=0; w<Score&&w<sc->GetChildCount(); w++)
			sc->GetChildNo(w)->Show();
		for (w=Score; w<sc->GetChildCount(); w++)
			sc->GetChildNo(w)->Hide();
		while (sc->GetChildCount()<Score)
		{
			PBox bx;
			bx.New();
			bx->SetDimensions(0.25);
			PDrawable d;
			d.New();
			d->SetShape( bx );
			d->MoveToX( 0.3 + 0.6 * sc->GetChildCount() );
			d->SetParent( sc );

			PMotionController mc;
			mc.New();
			mc->MotionInfo.AngVel.Set(4.0 * rand()/(float)RAND_MAX, 4.0 * rand()/(float)RAND_MAX,4.0 * rand()/(float)RAND_MAX);
			mc->SetObject(d);
			mc->Enable();
		}

		CTGroup* gm = p->Games;
		for (w=0; w<p->nGames&&w<gm->GetChildCount(); w++)
			gm->GetChildNo(w)->Show();
		for (; w<gm->GetChildCount(); w++)
			gm->GetChildNo(w)->Hide();
		float dg = 1.0;
		gm->MoveToX( -0.5 * ((p->nGames-1)*dg) );
		while (gm->GetChildCount()<p->nGames)
		{
			PBox bx;
			bx.New();
			bx->SetDimensions(0.75);
			PDrawable d;
			d.New();
			d->SetShape( bx );
			d->MoveToX( dg * gm->GetChildCount() );
			d->SetParent( gm );

			PMotionController mc;
			mc.New();
			mc->MotionInfo.AngVel.Set(4.0 * rand()/(float)RAND_MAX, 4.0 * rand()/(float)RAND_MAX,4.0 * rand()/(float)RAND_MAX);
			mc->SetObject(d);
			mc->Enable();
		}
	}

	for ( q=0; q<DSinas::Sini.Count(); q++)
	{
		DSinas* s = DSinas::Sini[q];
		if (!s->Viz)
			continue;
		PMotionController mc;
		mc.DynamicCast(s->Viz->GetController());
		if (mc)
			mc->MotionInfo.AngVel = s->AngVel;

		// In paused mode, hide uncontrolled players

		if (!s->Owner)
			continue;

#ifdef _XBOX
		bool hide = Waiting && (s->Owner->io.ap || !s->Owner->io.hwio || !s->Owner->io.hwio->GetD(Plugged));
#else
		bool hide = Waiting && s->Owner->io.autoPilot;
#endif
		if (hide)
			s->Viz->Hide();
		else
			s->Viz->Show();
	}

	// Nice cam pas above balls
	{
		DTreeClassIterator<CTCamera> cam(root);
		if (cam.Found(0))
		{
			Vector t;
			if (DoCamThing)
			{
				t.Zero();
				for (q=0;q<bl.Count();q++)
					t += bl[q]->Pos;
				t /= (float)bl.Count();
				t += Vector(0,20,0);
			}
			else
				t = Vector(0,20,-3);

			cam.Found(0)->MoveTo(t);
			Matrix mat;
			mat.V_PN = (Vector(0,-2,0) - t).Normalized();
			mat.V_RIGHT = (Vector(0,0,1)%mat.V_PN).Normalized();
			mat.V_UP = mat.V_PN%mat.V_RIGHT;

			cam.Found(0)->SetMatrix(mat);
		}
	}

}

void DGame::UpdateGame( float dt )
{
	if (Waiting)
		dt = 0.0f;

	CountDown -= dt;
	if (CountDown >= 0)
		dt = 0.0f;

	if (Winner>=0)
	{
		pl[Winner]->nGames++;
		LastWinner = Winner;
		Winner = -1;
		int bc = bl.Count();
		CreateBalls(0);
		CreateBalls(bc);
		Waiting = true;
		return;
	}

	// AutoPilot
	if (!_feqz(dt))
	{
		for (int q=0; q<pl.Count(); q++)
		{

#ifdef _XBOX
			if (pl[q]->io.hwio && pl[q]->io.hwio->GetD(Plugged))
				if (!pl[q]->io.ap)
					continue;
#else
			if (!pl[q]->io.autoPilot)
				continue;
#endif
			pl[q]->io.PlayMode = DPlayer::pmDigital;

			timers[q] += dt;
			if (timers[q]<0.3)
			{
				pl[q]->io.Pos = pl[q]->io.Neg = false;
				continue;
			}

			float nearest = 2.0;
			DBall* b = NULL;
			for (int w=0; w<bl.Count(); w++)
			{
				float v = bl[w]->Vel^pl[q]->Goal.normal;
				if (v>=0)
					continue;
				float p = pl[q]->Goal.Distance(bl[w]->Pos);
				p /= v;
				if (p>=nearest)
					continue;
				nearest = p;
				b = bl[w];
			}
			if (!b)
			{
				pl[q]->io.Pos = pl[q]->io.Neg = false;
				continue;
			}

			Vector lat = pl[q]->Goal.normal%Vector(0,1,0);
			bool dir = ((b->Pos - pl[q]->Bat.Pos)^lat)>0;

			if (pl[q]->io.Pos!=dir)
				timers[q] = 0.f;
			pl[q]->io.Pos = dir;
			pl[q]->io.Neg = !dir;
		}
	}

	// Do the game
	int q;
	for (q=0; q<pl.Count(); q++)
		pl[q]->UpdatePos(dt);
	for (q=0; q<bl.Count(); q++)
		bl[q]->UpdateBall(dt, pl);

}

void DGame::Interface( float dt )
{
#ifdef _XBOX
	// Once every 5 seconds, check for new controllers
	static float timer=0.5;
	int ot = timer*1.0;
	timer += dt;
	int t = timer*1.0;
	if (t!=ot)
	{
		bool oneok=false;
		for (int q=0; q<pl.Count(); q++)
		{
			if (pl[q]->io.hwio && pl[q]->io.hwio->IsStarted())
			{
				oneok = true;
				continue;
			}

			if (!pl[q]->io.hwio)
			{
				pl[q]->io.hwio = (CTHardwareIO*) citk_CreateClassByName("XGamePad");
				if (!pl[q]->io.hwio)
				{
					citk_Debug("Failed to create XGamePad class");
					continue;
				}
				pl[q]->io.hwio->SetDeviceID(q);
			}
			if (!pl[q]->io.hwio->Start())
			{
				pl[q]->io.hwio.Release();
				continue;
			}

//			if (!pl[q]->io.hwio.Acquire())
//				continue;
			pl[q]->io.hwio->Refresh();
			pl[q]->Score = pl[q]->MinScore = 0;
			pl[q]->Pos = pl[q]->Dir = 0;
		}
		if (!oneok && timer>10)
			Quit = true;
	}

	// do smt with controllers
	{
		for (int q=0; q<pl.Count(); q++)
		{
			CTHardwareIO* h = pl[q]->io.hwio;
			if (!h || !h->IsStarted() || !h->GetD(Plugged))
				continue;

			if (Waiting || CountDown>=0) // no regular updates
			{
				bool Shake = !Waiting && (fmod(CountDown, 1.0f)>0.8);
				h->SetA(LeftMotor,Shake?1:0);
				h->SetA(RightMotor,Shake?1:0);
				h->Refresh();
			}

			if (Waiting && h->GetD(Start))
			{
				Waiting = false;
				if (LastWinner>=0)
				{
					for (int q=0; q<pl.Count(); q++)
						pl[q]->Score = pl[q]->MinScore = 0;
					LastWinner = -1;
				}
				CountDown = 3.0;
			}

			if (h->GetA(A)>0.5)
				pl[q]->io.ap = false;

			if (h->GetA(Black)>0.5)
				DoCamThing = false;

			if (h->GetA(White)>0.5)
				DoCamThing = true;

			if (h->GetA(Y)>0.5 && h->GetA(B)>0.5)
				pl[q]->io.ap = true;

			if (h->GetD(Back))
				Waiting = true;

			if (h->GetD(LeftThumb)&&h->GetD(RightThumb))
				Quit = true;

			if (h->GetA(X)>0.5 && h->GetA(LeftTrigger)>0.5)
				CreateBalls(2);
			else if (h->GetA(X)>0.5 && h->GetA(RightTrigger)>0.5)
				CreateBalls(1);
		}
	}

#else
	if (kbhit())
	{
		int c = getch();
		if (c==27)
			Quit = true;
		if (c=='i')
			citk_ShowPropertyPage(root);
		if (c=='p')
			Waiting = true;
		if (c=='s')
		{
			Waiting = false;
			if (LastWinner>=0)
			{
				for (int q=0; q<pl.Count(); q++)
					pl[q]->Score = pl[q]->MinScore = 0;
				LastWinner = -1;
			}
			CountDown = 3.0;
		}
		if (c=='c')
			DoCamThing = !DoCamThing;
		if (c>='1' && c<='9')
			CreateBalls(c-'0');

		for (int q=0; q<pl.Count(); q++)
		{
			pl[q]->io.Pos = (c==MapPos[q]);
			pl[q]->io.Neg = (c==MapNeg[q]);
			if ( pl[q]->io.Pos || pl[q]->io.Neg )
				pl[q]->io.autoPilot = false;
		}
	}
	else
	{
		for (int q=0; q<pl.Count(); q++)
		{
			if (pl[q]->io.autoPilot)
				continue;

			pl[q]->io.Pos = pl[q]->io.Neg = false;
		}
	}
#endif
}

void PlayGame()
{
	DGame Game;

	Game.CreateBalls(1);
	Game.CreatePlayers(4);
	Game.CreateScene();
	citk_DoFrame();

	PServiceManager servMan;
	servMan.Acquire();
	citk_ShowPropertyPage(servMan);

	while (!Game.Quit)
	{
		float dt = TIME(citk_DoFrame());
#ifndef _XBOX
		printf("%5.1f fps      %6.4f    \r", 1.0/_max(0.001f,dt),dt);
#endif

		Game.Interface( dt );

		Game.UpdateGame( dt );

		Game.UpdateScene( dt );
	}
}

ulong citk::DTransform::timeid = 0;

class XBoxDebugFrontEnd : public CTFrontEnd
{
	DFile f;

public:
	XBoxDebugFrontEnd() { RegisterWithService(); }

	virtual void DebugString(cstr_t m) { if (!f.Opened()) f.Create(".\\log.txt"); f.WriteLine(m); }
	virtual void	ErrorManagerMessage ( errorstruct_t* error )
	{
		DebugString(error->msg);
	}
};

void __cdecl main()
{
	citk_Init();
	citk_LoadAllPlugIns();

	PDeviceManager devMan;
	devMan.Acquire();
	devMan->CreateAllDevices();

	new XBoxDebugFrontEnd;

	//REGISTER_CLASSES( Direct3D8 );
	//REGISTER_CLASSES( citkIO );

	citk_CreateClassByName("CTConsoleFrontEnd");

	PSceneCacheManager scm; scm->SetProperty("Path","files");
	PTextureManager tm; tm->SetProperty("Path", "files");

	PlayGame();

	citk_Deinit();
}

void CDECL citk_assert(cstr_t a) { puts(a);}

IMPLEMENT_COPYRIGHT( 0x003D5CDD, HBdB, "Bart de Boer", "bart.de.boer@crystalinter.com",
	"Crystal Intertechnology", "www.crystalinter.com" )

IMPLEMENT_COPYRIGHT( 0x8FB7F699, IC, "Ionut Cristea", "ionut.cristea@crystalinter.com",
	"Crystal Intertechnology", "www.crystalinter.com" )

IMPLEMENT_COPYRIGHT( 0x450177A5, LGGL, "Lionello Lunesu", "lionello.lunesu@crystalinter.com",
	"Crystal Intertechnology", "www.crystalinter.com" )

IMPLEMENT_COPYRIGHT( 0x68EF0E77, CS, "Catalin Slobodeanu", 
	"catalin.slobodeanu@crystalinter.com", "Crystal Interactive Systems", "" );


#include "Mu2eUtilities/inc/DriftFitUtils.hh"
#include "Mu2eUtilities/inc/ConvertXYZ.hh"
#include "Math/VectorUtil.h"
#include "TMath.h"
#include "Math/Math.h"
//Tracker Drift Conditions:
#include "TrackerGeom/inc/Tracker.hh"
#include "TrackerConditions/inc/StrawResponse.hh"
#include "TrackerConditions/inc/StrawPhysics.hh"
#include "TrackerConditions/inc/StrawDrift.hh"
//For Drift:
#include "BTrk/BaBar/BaBar.hh"
#include "BTrk/BbrGeom/Trajectory.hh"
#include "BTrk/KalmanTrack/KalRep.hh"
#include "BTrk/BbrGeom/HepPoint.h"
#include "BTrk/TrkBase/TrkPoca.hh"
#include "BTrkData/inc/TrkStrawHit.hh"
#include "BTrk/BbrGeom/BbrVectorErr.hh"
#include "BTrk/TrkBase/TrkPoca.hh"
#include "BTrk/ProbTools/ChisqConsistency.hh"
#include "BTrk/TrkBase/TrkMomCalculator.hh"

using namespace mu2e;

TrkPoca DriftFitUtils::GetPOCA(Straw const& straw, double a0, double a1, double b0, double b1, ComboHit chit) {
		//cout<<"in poca "<<endl;
		XYZVec track_position(a0,b0,0);
		XYZVec track_direction(a1,b1,1);
		track_direction= track_direction.Unit();
		TrackAxes MinuitCoordSystem = ParametricFit::GetTrackAxes(track_direction);
		cout<<"Track axes being used "<<MinuitCoordSystem._XDoublePrime<<" "<<MinuitCoordSystem._YDoublePrime<<" "<<MinuitCoordSystem._ZPrime<<endl;
      		const CLHEP::Hep3Vector& spos = straw.getMidPoint();
      		const CLHEP::Hep3Vector& sdir = straw.getDirection();
      		
 		CLHEP::Hep3Vector wpos = spos + chit.wireDist()*sdir;
 		//cout<<"getting wiore details "<<endl;
      		XYZVec wire_position = ConvertToXYZ(wpos);
                XYZVec wire_direction= ConvertToXYZ(sdir);
                //cout<<"getting primes "<<endl;
                XYZVec wire_prime_position(wire_position.Dot(MinuitCoordSystem._XDoublePrime),wire_position.Dot(MinuitCoordSystem._YDoublePrime),wire_position.Dot(MinuitCoordSystem._ZPrime));
                XYZVec wire_prime_direction(wire_direction.Dot(MinuitCoordSystem._XDoublePrime), wire_direction.Dot(MinuitCoordSystem._YDoublePrime), wire_direction.Dot(MinuitCoordSystem._ZPrime));
      		//cout<<"converting "<<endl;
      		HepPoint pointwire = ConvertToHepPoint(wire_prime_position);
      		HepPoint tpos = ConvertToHepPoint(track_position);
      		Hep3Vector tdir = ConvertToHep3Vector(track_direction);
      		Hep3Vector wdir = ConvertToHep3Vector(wire_prime_direction);
                //cout<<"flight  legnths "<<endl;     
      		double fltlen = (fabs((spos.z()-tpos.z())/tdir.z())); //along the track = spos.z() in  prime frame
	 	double hitlen = fabs(wdir.dot(tpos - spos)); //distance from track to wire centre, i.e dist along the straw
	 	//cout<<"trajs "<<endl;
      		TrkLineTraj wire(pointwire,wdir);//chit.wireDist()-chit.wireRes(),chit.wireDist()+chit.wireRes());
      		TrkLineTraj track(tpos,tdir);
      		
      		TrkPoca hitpoca(track,fltlen,wire,hitlen);
      		return hitpoca;
     }
//could use ParameticFit functions but not yet able
std::vector<double> DriftFitUtils::UpdateErrors(double a0, double a1, double b0, double b1, ComboHit chit) {
		
		XYZVec track_direction(a1,b1,1);
		track_direction= track_direction.Unit();
		std::vector<XYZVec> AxesList= ParametricFit::GetAxes(track_direction);
		XYZVec const& wdir = chit.wdir();//direction along wire
      		double werr_mag = chit.wireRes(); //hit major error axis  
      		XYZVec major_axis = werr_mag*wdir;
      		XYZVec wtdir = Geom::ZDir().Cross(wdir); // transverse direction to the wire 
      		double terr_mag = chit.transRes(); //hit minor error axis
      		XYZVec minor_axis = terr_mag*wtdir;
        	double sigma_w_squared = major_axis.Mag2();      
        	double sigma_v_squared = minor_axis.Mag2();
		double sigma_x_track = sqrt(sigma_w_squared*pow(AxesList[0].Dot(major_axis.Unit()),2)+sigma_v_squared*pow(AxesList[0].Dot(minor_axis.Unit()),2));
        	double sigma_y_track = sqrt(sigma_w_squared*pow((AxesList[1].Dot(major_axis.Unit())),2)+sigma_v_squared*pow((AxesList[1].Dot(minor_axis.Unit())),2));
        	std::vector<double> ErrorsXY;
        	ErrorsXY.push_back(sigma_x_track);
        	ErrorsXY.push_back(sigma_y_track);
		return ErrorsXY;
     }
	
	
double DriftFitUtils::GetDOCA(TrkPoca hitpoca) {
        	double doca = hitpoca.doca();
      		return doca;
}
	
double DriftFitUtils::GetPropVelocity(StrawResponse rep, ComboHit chit){
	   	double vprop = 2.0*rep.halfPropV(chit.strawId(),1000.0*chit.energyDep());
	   	return vprop; 
}

double DriftFitUtils::GetPropTime(ComboHit chit, Straw straw, double vprop) {
	    double tprop = 0.;
	    //if(poca.status().success()){
	      switch (chit.driftEnd()) {
		case StrawEnd::cal:
		  tprop = (straw.halfLength()+chit.wireDist())/vprop;
		  break;
		case StrawEnd::hv:
		  tprop = (straw.halfLength()-chit.wireDist())/vprop;
		  break;
	      }
	    
	    return tprop;
  }

double DriftFitUtils::TimeResidualTrans(Straw const&  straw, double doca, StrawResponse srep, double t0, ComboHit hit){
	        
                StrawId strawid = straw.id();
      		double phi =  hit.phi();
      	        double drift_time= srep.StrawResponse::driftDistanceToTime(strawid , fabs(doca), phi);
      	        return drift_time;
}
        
double DriftFitUtils::TimeResidualLong(Straw const&  straw, double doca, StrawResponse srep,  double t0, ComboHit hit){
		
                StrawId strawid = straw.id();
      	        double _vprop = 2.0*srep.StrawResponse::halfPropV(strawid,1000.0*hit.energyDep());
      	        double propagation_time = GetPropTime(hit, straw,_vprop);
      	        //double propagation_time = straw.halfLength()/_vprop;
      	        return propagation_time;
}

double DriftFitUtils::TimeResidual(Straw const&  straw, double doca, StrawResponse srep, double t0, ComboHit hit){
		double time_residual_long = TimeResidualLong( straw,  doca, srep,  t0,  hit);
		double time_residual_trans = TimeResidualTrans(straw,doca, srep, t0, hit); 
		//std::cout<<hit.time()<<"trans "<<time_residual_trans<<" long time "<<time_residual_long<<std::endl;
	      	std::cout<<"Time Residual "<<time_residual_trans + time_residual_long<<std::endl;
		return time_residual_trans + time_residual_long;
}

double DriftFitUtils::T0(Straw const&  straw, double doca, StrawResponse srep, double t0, ComboHit hit, double Aver){
		double time_residual_long = TimeResidualLong( straw,  doca, srep,  t0,  hit);
		double time_residual_trans = TimeResidualTrans(straw,doca, srep, t0, hit); 
		Aver += hit.time() - time_residual_trans - time_residual_long;
		return Aver; //hit.time() - time_residual_trans - time_residual_long;
}


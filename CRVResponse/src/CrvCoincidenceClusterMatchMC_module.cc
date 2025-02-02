//
// A module to find the MC information for the clusters of coincidences of CRV pulses
//
//
// Original Author: Ralf Ehrlich

#include "Offline/CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "Offline/DataProducts/inc/CRSScintillatorBarIndex.hh"

#include "Offline/CRVResponse/inc/CrvHelper.hh"
#include "Offline/ConditionsService/inc/AcceleratorParams.hh"
#include "Offline/ConditionsService/inc/ConditionsHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/GeometryService.hh"
#include "Offline/MCDataProducts/inc/GenParticle.hh"
#include "Offline/MCDataProducts/inc/CrvDigiMC.hh"
#include "Offline/MCDataProducts/inc/CrvCoincidenceClusterMC.hh"
#include "Offline/RecoDataProducts/inc/CrvRecoPulse.hh"
#include "Offline/RecoDataProducts/inc/CrvCoincidenceCluster.hh"
#include "Offline/Mu2eUtilities/inc/SimParticleTimeOffset.hh"

#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "fhiclcpp/ParameterSet.h"
#include "CLHEP/Units/GlobalSystemOfUnits.h"

#include <string>

#include <TMath.h>
#include <TH2D.h>

namespace mu2e
{
  class CrvCoincidenceClusterMatchMC : public art::EDProducer
  {
    public:
    explicit CrvCoincidenceClusterMatchMC(fhicl::ParameterSet const& pset);
    void produce(art::Event& e);
    void beginJob();
    void beginRun(art::Run &run);
    void endJob();

    private:
    std::string _crvCoincidenceClusterFinderModuleLabel;  //module label of the CrvCoincidenceClusterFinder module
                                                          //it is possible to have more than one instance of the CrvCoincidenceClusterFinder module
    std::string _crvWaveformsModuleLabel;  //module label of the CrvWaveform module.
                                           //this is optional. only needed, if MC information is required
    SimParticleTimeOffset _timeOffsets;
  };

  CrvCoincidenceClusterMatchMC::CrvCoincidenceClusterMatchMC(fhicl::ParameterSet const& pset) :
   art::EDProducer{pset},
    _crvCoincidenceClusterFinderModuleLabel(pset.get<std::string>("crvCoincidenceClusterFinderModuleLabel")),
    _crvWaveformsModuleLabel(pset.get<std::string>("crvWaveformsModuleLabel","")),
    _timeOffsets(pset.get<fhicl::ParameterSet>("timeOffsets"))
  {
    produces<CrvCoincidenceClusterMCCollection>();
  }

  void CrvCoincidenceClusterMatchMC::beginJob()
  {
  }

  void CrvCoincidenceClusterMatchMC::endJob()
  {
  }

  void CrvCoincidenceClusterMatchMC::beginRun(art::Run &run)
  {
  }

  void CrvCoincidenceClusterMatchMC::produce(art::Event& event)
  {
    _timeOffsets.updateMap(event);

    std::unique_ptr<CrvCoincidenceClusterMCCollection> crvCoincidenceClusterMCCollection(new CrvCoincidenceClusterMCCollection);

    art::Handle<CrvCoincidenceClusterCollection> crvCoincidenceClusterCollection;
    event.getByLabel(_crvCoincidenceClusterFinderModuleLabel,"",crvCoincidenceClusterCollection);

    if(crvCoincidenceClusterCollection.product()==NULL) return;

    art::Handle<CrvDigiMCCollection> crvDigiMCCollection;
    if(_crvWaveformsModuleLabel!="") event.getByLabel(_crvWaveformsModuleLabel,"",crvDigiMCCollection); //this is an optional part for MC information

    CrvCoincidenceClusterCollection::const_iterator iter;
    for(iter=crvCoincidenceClusterCollection->begin(); iter!=crvCoincidenceClusterCollection->end(); iter++)
    {
      bool   hasMCInfo                = (crvDigiMCCollection.isValid()?true:false); //MC
      double visibleEnergyDeposited   = 0;         //MC
      double earliestHitTime          = NAN;       //MC
      art::Ptr<SimParticle> simParticle;           //MC
      CLHEP::Hep3Vector     earliestHitPos;        //MC

      //loop through all reco pulses and try to find the MC information
      std::vector<CrvCoincidenceClusterMC::PulseInfo> pulses; //collection of all pulses (sim particle, energy dep.)
      std::set<art::Ptr<CrvStep> > steps;  //collection of all crvSteps for total energy calculation
                                           //use a set to avoid double counts, e.g. for two pulses coming from same digi
      const std::vector<art::Ptr<CrvRecoPulse> > &crvRecoPulses = iter->GetCrvRecoPulses();
      for(size_t i=0; i<crvRecoPulses.size(); i++)
      {
        const art::Ptr<CrvRecoPulse> crvRecoPulse = crvRecoPulses[i];
        art::Ptr<SimParticle> simParticleThisPulse;
        double visibleEnergyDepositedThisPulse = 0;
        double earliestHitTimeThisPulse = NAN; //not used here
        CLHEP::Hep3Vector earliestHitPosThisPulse; //not used here

        //get MC information, if available
        if(hasMCInfo)
        {
          //get the step points of this reco pulse and add it to the collection of all step points
          CrvHelper::GetStepPointsFromCrvRecoPulse(crvRecoPulse, crvDigiMCCollection, steps);

          //get the sim particle and deposited energy of this reco pulse
          CrvHelper::GetInfoFromCrvRecoPulse(crvRecoPulse, crvDigiMCCollection, _timeOffsets,
                                             visibleEnergyDepositedThisPulse,
                                             earliestHitTimeThisPulse, earliestHitPosThisPulse, simParticleThisPulse);
        }

        //add the MC information (sim particle, dep. energy) of this reco pulse to the collection of pulses
        //unless the reco pulse was caused by a noise hit (and doesn't have an associated simParticle)
        if(simParticleThisPulse.isAvailable()) pulses.emplace_back(simParticleThisPulse,visibleEnergyDepositedThisPulse);
      }//loop over reco pulses

      //based on all step points, get the most likely sim particle, total energy, etc.
      CrvHelper::GetInfoFromStepPoints(steps, _timeOffsets,
                                       visibleEnergyDeposited, earliestHitTime, earliestHitPos, simParticle);

      //insert the cluster information into the vector of the crv coincidence clusters (collection of pulses, most likely sim particle, etc.)
      //unless the reco pulse was caused by a noise hit (and doesn't have an associated simParticle)
      if(simParticle.isAvailable()) crvCoincidenceClusterMCCollection->emplace_back(hasMCInfo, pulses, simParticle,
                                                                                    visibleEnergyDeposited, earliestHitTime, earliestHitPos);
    }//loop over all clusters

    event.put(std::move(crvCoincidenceClusterMCCollection));
  } // end produce

} // end namespace mu2e

using mu2e::CrvCoincidenceClusterMatchMC;
DEFINE_ART_MODULE(CrvCoincidenceClusterMatchMC)

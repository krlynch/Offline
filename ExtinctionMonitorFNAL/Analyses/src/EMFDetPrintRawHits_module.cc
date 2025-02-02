// Printout ExtMonFNAL raw hits
//
// Andrei Gaponenko, 2012

#include "Offline/RecoDataProducts/inc/ExtMonFNALRawHit.hh"


#include "Offline/ExtinctionMonitorFNAL/Analyses/inc/GenericCollectionPrinter.hh"

namespace mu2e {

  class EMFDetPrintRawHits : public GenericCollectionPrinter<ExtMonFNALRawHitCollection> {
  public:
    explicit EMFDetPrintRawHits(const fhicl::ParameterSet& pset)
      : GenericCollectionPrinter(pset)
    {}

  };

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::EMFDetPrintRawHits);

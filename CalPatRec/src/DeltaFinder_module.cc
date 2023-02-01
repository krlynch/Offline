/////////////////////////////////////////////////////////////////////////////
// framework
//
// parameter defaults: CalPatRec/fcl/prolog.fcl
//////////////////////////////////////////////////////////////////////////////
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/EDProducer.h"
#include "art_root_io/TFileService.h"

#include "art/Utilities/make_tool.h"
#include "Offline/Mu2eUtilities/inc/ModuleHistToolBase.hh"

#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "Offline/TrackerGeom/inc/Tracker.hh"
#include "Offline/CalorimeterGeom/inc/DiskCalorimeter.hh"

// conditions
#include "Offline/ConditionsService/inc/ConditionsHandle.hh"

// data
#include "Offline/RecoDataProducts/inc/ComboHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHitPosition.hh"
#include "Offline/RecoDataProducts/inc/StereoHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHitFlag.hh"
#include "Offline/RecoDataProducts/inc/CaloCluster.hh"
#include "Offline/RecoDataProducts/inc/TimeCluster.hh"

// diagnostics

#include "Offline/CalPatRec/inc/DeltaFinder_types.hh"
#include "Offline/CalPatRec/inc/DeltaFinderAlg.hh"

#include <algorithm>
#include <cmath>

using namespace std;

namespace mu2e {

  using namespace DeltaFinderTypes;

  class DeltaFinder: public art::EDProducer {
  public:

    struct Config {
      using Name    = fhicl::Name;
      using Comment = fhicl::Comment;
      fhicl::Atom<art::InputTag>   shCollTag         {Name("shCollTag"         ), Comment("SComboHit collection Name"   ) };
      fhicl::Atom<art::InputTag>   chCollTag         {Name("chCollTag"         ), Comment("ComboHit collection Name"    ) };
      fhicl::Atom<art::InputTag>   sdmcCollTag       {Name("sdmcCollTag"       ), Comment("StrawDigiMC collection Name" ) };
      fhicl::Atom<art::InputTag>   tpeakCollTag      {Name("tpeakCollTag"      ), Comment("Time peak collection Name"   ) };
      fhicl::Atom<int>             useTimePeaks      {Name("useTimePeaks"      ), Comment("to use time peaks set to 1"  ) };
      fhicl::Atom<int>             debugLevel        {Name("debugLevel"        ), Comment("debug level"                 ) };
      fhicl::Atom<int>             diagLevel         {Name("diagLevel"         ), Comment("diag level"                  ) };
      fhicl::Atom<int>             printErrors       {Name("printErrors"       ), Comment("print errors"                ) };
      fhicl::Atom<float>           minCaloDt         {Name("minCaloDt"         ), Comment("min Calo Dt"                 ) };
      fhicl::Atom<float>           maxCaloDt         {Name("maxCaloDt"         ), Comment("max Calo Dt"                 ) };
      fhicl::Atom<float>           meanPitchAngle    {Name("meanPitchAngle"    ), Comment("mean pitch angle"            ) };
      fhicl::Atom<float>           minHitTime        {Name("minHitTime"        ), Comment("min hit time"                ) };
      fhicl::Atom<float>           maxDeltaEDep      {Name("maxDeltaEDep"      ), Comment("max delta candidate  eDep"   ) };
      fhicl::Atom<float>           maxSeedEDep       {Name("maxSeedEDep"       ), Comment("max seed eDep"               ) };
      fhicl::Atom<float>           minProtonSeedEDep {Name("minProtonSeedEDep" ), Comment("min proton seed eDep"        ) };
      fhicl::Atom<int>             minNSeeds         {Name("minNSeeds"         ), Comment("min N seeds in a delta cand" ) };
      fhicl::Atom<int>             minDeltaNHits     {Name("minDeltaNHits"     ), Comment("min N combo  hits in a delta") };
      fhicl::Atom<float>           maxEleHitEnergy   {Name("maxEleHitEnergy"   ), Comment("max electron hit energy"     ) };
      fhicl::Atom<float>           minimumTime       {Name("minimumTime"       ), Comment("minimum time"                ) };
      fhicl::Atom<float>           maximumTime       {Name("maximumTime"       ), Comment("maximum time"                ) };
      fhicl::Atom<float>           maxHitSeedDt      {Name("maxHitSeedDt"      ), Comment("max DT(hit-seed)"            ) };
      fhicl::Atom<float>           maxChi2Seed       {Name("maxChi2Seed"       ), Comment("max seed chi2 (stereo)"      ) };
      fhicl::Atom<float>           maxChi2Radial     {Name("maxChi2Radial"     ), Comment("max chi2 (radial)"           ) };
      fhicl::Atom<float>           maxChi2All        {Name("maxChi2All"        ), Comment("max chi2 (all)"              ) };
      fhicl::Atom<float>           maxChi2SeedDelta  {Name("maxChi2SeedDelta"  ), Comment("max chi2 (seed-delta)"       ) };
      fhicl::Atom<float>           seedRes           {Name("seedRes"           ), Comment("stereo seed resolution"      ) };
      fhicl::Atom<float>           maxDxy            {Name("maxDxy"            ), Comment("max Dxy"                     ) };
      fhicl::Atom<int>             maxGap            {Name("maxGap"            ), Comment("max Gap"                     ) };
      fhicl::Atom<float>           sigmaR            {Name("sigmaR"            ), Comment("sigmaR"                      ) };
      fhicl::Atom<float>           maxDriftTime      {Name("maxDriftTime"      ), Comment("maxDriftTime"                ) };
      fhicl::Atom<float>           maxSeedDt         {Name("maxSeedDt"         ), Comment("maxSeedDt"                   ) };
      fhicl::Atom<float>           maxHitDt          {Name("maxHitDt"          ), Comment("maxHitDt"                    ) };
      fhicl::Atom<float>           maxStrawDt        {Name("maxStrawDt"        ), Comment("max straw Dt"                ) };
      fhicl::Atom<float>           maxDtDs           {Name("maxDtDs"           ), Comment("max Dt/Dstation"             ) };
      fhicl::Atom<float>           maxDtDc           {Name("maxDtDc"           ), Comment("max deltaT between deltas"   ) };
      fhicl::Atom<int>             writeComboHits    {Name("writeComboHits"    ), Comment("if 1, write combohit coll"   ) };
      fhicl::Atom<int>             writeStrawHitFlags{Name("writeStrawHitFlags"), Comment("if 1, write SH flag coll"    ) };
      fhicl::Atom<int>             testOrder         {Name("testOrder"         ), Comment("if 1, test order"            ) };
      fhicl::Atom<bool>            testHitMask       {Name("testHitMask"       ), Comment("if true, test hit mask"      ) };
      fhicl::Sequence<std::string> goodHitMask       {Name("goodHitMask"       ), Comment("good hit mask"               ) };
      fhicl::Sequence<std::string> bkgHitMask        {Name("bkgHitMask"        ), Comment("background hit mask"         ) };
      fhicl::Atom<int>             updateSeedCOG     {Name("updateSeedCOG"     ), Comment("if 1, update seed COG"       ) };

      fhicl::Table<DeltaFinderTypes::Config> diagPlugin      {Name("diagPlugin"      ), Comment("Diag plugin"           ) };
      fhicl::Table<DeltaFinderAlg::Config>   finderParameters{Name("finderParameters"), Comment("finder alg parameters" ) };
    };

  protected:
//-----------------------------------------------------------------------------
// talk-to parameters: input collections and algorithm parameters
//-----------------------------------------------------------------------------
    art::InputTag   _shCollTag;
    art::InputTag   _chCollTag;
    art::InputTag   _sdmcCollTag;
    art::InputTag   _tpeakCollTag;

    int             _useTimePeaks;
    float           _minCaloDt;
    float           _maxCaloDt;
    float           _meanPitchAngle;

    float           _timeBinWidth;
    float           _minHitTime;           // min hit time
    float           _maxDeltaEDep;         //
    float           _maxSeedEDep;          //
    float           _minProtonSeedEDep;    //
    int             _minNSeeds;            // min number of seeds in the delta electron cluster
    int             _minDeltaNHits;        // min number of hits of a delta candidate
    float           _maxEleHitEnergy;      //
    float           _minT;
    float           _maxT;
    float           _maxHitSeedDt;         //
    float           _maxChi2Seed;          //
    float           _maxChi2Neighbor;      //
    float           _maxChi2Radial;        //
    float           _maxChi2All;           // max chi2/N of a seed
    float           _maxChi2SeedDelta;     // max delta-seed chi2 for adding a seed to a delta
    float           _seedRes;              //
    float           _maxDxy;
    int             _maxGap;
    float           _sigmaR;
    float           _sigmaR2;              // _sigmaR^2
    float           _maxDriftTime;
    float           _maxSeedDt;            // +/- SeedDt is the time window for checking duplicate seeds
    float           _maxHitDt;
    float           _maxStrawDt;
    float           _maxDtDs;              // low-P electron travel time between two stations
    float           _maxDtDc;              // max deltaT between two delta candiates

    int             _writeComboHits;       // write (filtered ?) combo hits
    int             _writeStrawHitFlags;

    int             _debugLevel;
    int             _diagLevel;
    int             _printErrors;
    int             _testOrder;
    bool            _testHitMask;
    StrawHitFlag    _goodHitMask;
    StrawHitFlag    _bkgHitMask;
    int             _updateSeedCOG;

    std::unique_ptr<ModuleHistToolBase> _hmanager;
//-----------------------------------------------------------------------------
// cache event/geometry objects
//-----------------------------------------------------------------------------
    const StrawHitCollection*    _shColl ;

    const Tracker*               _tracker;
    const DiskCalorimeter*       _calorimeter;

    float                        _tdbuff; // following Dave - time division buffer

    DeltaFinderTypes::Data_t     _data;              // all data used
    int                          _testOrderPrinted;

    DeltaFinderAlg*              _finder;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  public:
    explicit DeltaFinder(const art::EDProducer::Table<Config>& config);

  private:

    bool         findData            (const art::Event&  Evt);
    int          checkDuplicates     (int Station,
                                      int Face1, const HitData_t* Hit1,
                                      int Face2, const HitData_t* Hit2);

    void         completeSeed        (DeltaSeed* Seed);
    void         connectSeeds        ();
    void         findSeeds           (int Station, int Face);
    void         findSeeds           ();
    void         initTimeCluster     (DeltaCandidate* Delta, TimeCluster* Tc);
    int          mergeDeltaCandidates();
    // int          orderHits           ();
    void         pruneSeeds          (int Station);
    int          recoverMissingHits  ();
    int          recoverSeed         (DeltaCandidate* Delta, int LastStation, int Station);
    int          recoverStation      (DeltaCandidate* Delta, int LastStation, int Station, int UseUsedHits, int RecoverSeeds);
    void         runDeltaFinder      ();
    //    double       seedDeltaChi2       (DeltaSeed* Seed, DeltaCandidate* Delta);
//-----------------------------------------------------------------------------
// overloaded methods of the module class
//-----------------------------------------------------------------------------
    void         beginJob() override;
    void         beginRun(art::Run& ARun) override;
    void         endJob  () override;
    void         produce (art::Event& E ) override;
  };

//-----------------------------------------------------------------------------
  DeltaFinder::DeltaFinder(const art::EDProducer::Table<Config>& config):
    art::EDProducer{config},
    _shCollTag             (config().shCollTag()        ),
    _chCollTag             (config().chCollTag()        ),
    _sdmcCollTag           (config().sdmcCollTag()      ),
    _tpeakCollTag          (config().tpeakCollTag()     ),
    _useTimePeaks          (config().useTimePeaks()     ),
    _minCaloDt             (config().minCaloDt()        ),
    _maxCaloDt             (config().maxCaloDt()        ),
    _meanPitchAngle        (config().meanPitchAngle()   ),
    _minHitTime            (config().minHitTime()       ),
    _maxDeltaEDep          (config().maxDeltaEDep()     ),
    _maxSeedEDep           (config().maxSeedEDep()      ),
    _minProtonSeedEDep     (config().minProtonSeedEDep()),
    _minNSeeds             (config().minNSeeds()        ),
    _minDeltaNHits         (config().minDeltaNHits()    ),
    _maxEleHitEnergy       (config().maxEleHitEnergy()  ),
    _minT                  (config().minimumTime()      ), // nsec
    _maxT                  (config().maximumTime()      ), // nsec
    _maxHitSeedDt          (config().maxHitSeedDt()     ), // nsec
    _maxChi2Seed           (config().maxChi2Seed()      ),
    _maxChi2Radial         (config().maxChi2Radial()    ),
    _maxChi2All            (config().maxChi2All()       ),
    _maxChi2SeedDelta      (config().maxChi2SeedDelta() ),
    _seedRes               (config().seedRes()          ),
    _maxDxy                (config().maxDxy()           ),
    _maxGap                (config().maxGap()           ),
    _sigmaR                (config().sigmaR()           ),
    _maxDriftTime          (config().maxDriftTime()     ),
    _maxSeedDt             (config().maxSeedDt()        ),
    _maxHitDt              (config().maxHitDt()         ),
    _maxStrawDt            (config().maxStrawDt()       ),
    _maxDtDs               (config().maxDtDs()          ),
    _maxDtDc               (config().maxDtDc()          ),
    _writeComboHits        (config().writeComboHits()   ),
    _writeStrawHitFlags    (config().writeStrawHitFlags()),
    _debugLevel            (config().debugLevel()        ),
    _diagLevel             (config().diagLevel()         ),
    _printErrors           (config().printErrors()       ),
    _testOrder             (config().testOrder()         ),
    _testHitMask           (config().testHitMask()       ),
    _goodHitMask           (config().goodHitMask()       ),
    _bkgHitMask            (config().bkgHitMask()        ),
    _updateSeedCOG         (config().updateSeedCOG()     )
  {
    consumesMany<ComboHitCollection>(); // Necessary because fillStrawHitIndices calls getManyByType.

    produces<StrawHitFlagCollection>("ComboHits");
    if (_writeStrawHitFlags == 1) produces<StrawHitFlagCollection>("StrawHits");
    if (_writeComboHits     == 1) produces<ComboHitCollection>    ("");

                                        // this is a list of delta-electron candidates
    produces<TimeClusterCollection>();

    _finder = new DeltaFinderAlg(config().finderParameters,&_data);

    _testOrderPrinted = 0;
    _tdbuff           = 80.;                 // mm ... about less than 1 ns
    _sigmaR2          = _sigmaR*_sigmaR;

    if (_diagLevel != 0) _hmanager = art::make_tool  <ModuleHistToolBase>(config().diagPlugin.get_PSet());
    else                 _hmanager = std::make_unique<ModuleHistToolBase>();

    _data.chCollTag      = _chCollTag;
    _data.sdmcCollTag    = _sdmcCollTag;
    _data.meanPitchAngle = _meanPitchAngle;

    _timeBinWidth        = 50.;       // ns

  }

  //-----------------------------------------------------------------------------
  void DeltaFinder::beginJob() {
    if (_diagLevel > 0) {
      art::ServiceHandle<art::TFileService> tfs;
      _hmanager->bookHistograms(tfs);
    }
  }

  //-----------------------------------------------------------------------------
  void DeltaFinder::endJob() {
  }

//-----------------------------------------------------------------------------
// create a Z-ordered representation of the tracker
//-----------------------------------------------------------------------------
  void DeltaFinder::beginRun(art::Run& aRun) {

    _data.InitGeometry();
//-----------------------------------------------------------------------------
// it is enough to print that once
//-----------------------------------------------------------------------------
    if (_testOrder && (_testOrderPrinted == 0)) {
      _data.testOrderID  ();
      _data.testdeOrderID();
      _testOrderPrinted = 1;
    }

    if (_diagLevel != 0) _hmanager->debug(&_data,1);
  }

//-----------------------------------------------------------------------------
// make sure the two hits used to make a new seed are not a part of an already found seed
//-----------------------------------------------------------------------------
  int DeltaFinder::checkDuplicates(int Station, int Face1, const HitData_t* Hit1, int Face2, const HitData_t* Hit2) {

    int rc(0);

    int nseeds = _data.NSeeds(Station);
    for (int i=0; i<nseeds; i++) {
      DeltaSeed* seed = _data.deltaSeed(Station,i);

      if ((seed->hitData[Face1] == Hit1) and (seed->hitData[Face2] == Hit2)) {
//-----------------------------------------------------------------------------
// 'seed' contains both Hit1 and Hit2, done
//-----------------------------------------------------------------------------
        rc = 1;
                                                                          break;
      }
    }
    return rc;
  }

//-----------------------------------------------------------------------------
// input 'Seed' has two hits , non parallel wires
// try to add more close hits to it (one hit per face)
//-----------------------------------------------------------------------------
  void DeltaFinder::completeSeed(DeltaSeed* Seed) {

    assert ((Seed->SFace(1) >= 0) and (Seed->NHits() == 2));
//-----------------------------------------------------------------------------
// loop over remaining faces, 'f2' - face in question
//-----------------------------------------------------------------------------
    int station = Seed->Station();

    float xseed = Seed->Xc ();
    float yseed = Seed->Yc ();
    float rho   = sqrt(xseed*xseed+yseed*yseed);
    float nxs   = xseed/rho;
    float nys   = yseed/rho;

    for (int face=0; face<kNFaces; face++) {
      if (Seed->fFaceProcessed[face] == 1)                            continue;
//-----------------------------------------------------------------------------
// face is different from the two first faces used
//-----------------------------------------------------------------------------
      for (int p2=0; p2<3; ++p2) {
        PanelZ_t* pz = &_data.oTracker[station][face][p2];

        float n1n2 = nxs*pz->nx+nys*pz->ny;
        if (n1n2 < 0.5)                                               continue;
//-----------------------------------------------------------------------------
// panel overlaps with the seed, look at its hits
//-----------------------------------------------------------------------------
        double pwx      = pz->wx;
        double pwy      = pz->wy;

        float sxy_dot_w = xseed*pwx+yseed*pwy;
        if (sxy_dot_w > 1.e10) printf("emoe! 001 \n");

        HitData_t* closest_hit(nullptr);
        float      best_chi2(_maxChi2Radial);

        int    nhits = pz->fHitData->size();
        for (int ih=0; ih<nhits; ih++) {
//-----------------------------------------------------------------------------
// 2017-10-05 PM: consider all hits
// hit time should be consistent with the already existing times - the difference
// between any two measured hit times should not exceed _maxDriftTime
// (_maxDriftTime represents the maximal drift time in the straw, should there be some tolerance?)
//-----------------------------------------------------------------------------
          HitData_t* hd      = &(*pz->fHitData)[ih];
          float corr_time    = hd->fCorrTime;

          if (corr_time-Seed->T0Max() > _maxHitSeedDt)                break;
          if (Seed->T0Min()-corr_time > _maxHitSeedDt)                continue;

          const ComboHit* ch = hd->fHit;
          double dx          = ch->pos().x()-xseed;
          double dy          = ch->pos().y()-yseed;
//-----------------------------------------------------------------------------
// split into wire parallel and perpendicular components
// assume wire direction vector is normalized to 1
//-----------------------------------------------------------------------------
          float dxy_dot_wdir = dx*pwx+dy*pwy;

          float dx_perp      = dx-dxy_dot_wdir*pwx;
          float dy_perp      = dy-dxy_dot_wdir*pwy;
          float d_perp_2     = dx_perp*dx_perp+dy_perp*dy_perp;

          // add an uncertainty on the intersection, can do better :

          float chi2_par     = (dxy_dot_wdir*dxy_dot_wdir)/(hd->fSigW2+_seedRes*_seedRes);
          float chi2_perp    = d_perp_2/_sigmaR2;
          float chi2         = chi2_par + chi2_perp;

          if (_updateSeedCOG != 0) {
//-----------------------------------------------------------------------------
// try updating the seed candidate coordinates with the hit added
// to see if that could speed the code up by improvind the efficiency
// of picking up the right hits
// OFF by default
//-----------------------------------------------------------------------------
            double nr   = ch->pos().x()*pwy-ch->pos().y()*pwx;

            double nx2m = (Seed->fSnx2+pwx*pwx)/(Seed->NHits()+1);
            double nxym = (Seed->fSnxy+pwx*pwy)/(Seed->NHits()+1);
            double ny2m = (Seed->fSny2+pwy*pwy)/(Seed->NHits()+1);
            double nxrm = (Seed->fSnxr+pwx*nr )/(Seed->NHits()+1);
            double nyrm = (Seed->fSnyr+pwy*nr )/(Seed->NHits()+1);

            double d    = nx2m*ny2m-nxym*nxym;

            double xc   = (nyrm*nx2m-nxrm*nxym)/d;
            double yc   = (nyrm*nxym-nxrm*ny2m)/d;

            float dx1   = ch->pos().x()-xc;
            float dy1   = ch->pos().y()-yc;

            float dxy1_dot_wdir = dx1*pwx+dy1*pwy;

            float dx1_perp      = dx1-dxy1_dot_wdir*pwx;
            float dy1_perp      = dy1-dxy1_dot_wdir*pwy;
            float dxy1_perp_2   = dx1_perp*dx1_perp+dy1_perp*dy1_perp;

            double chi2_par1(0), chi2_perp1(0);

            Seed->Chi2(xc,yc,_sigmaR2,chi2_par1,chi2_perp1);
            chi2_par1          += (dxy1_dot_wdir*dxy1_dot_wdir)/(hd->fSigW2 + _sigmaR2);
            chi2_perp1         += dxy1_perp_2/_sigmaR2;                        // some radius
            float chi21         = (chi2_par1 + chi2_perp1)/(Seed->NHits()+1);
            chi2                = chi21;
          }

          if (chi2 < best_chi2) {
                                        // new best hit
            closest_hit = hd;
            best_chi2   = chi2;
          }
        }

        if (closest_hit) {
//-----------------------------------------------------------------------------
// add hit
//-----------------------------------------------------------------------------
          closest_hit->fChi2Min = best_chi2;
          Seed->AddHit(closest_hit,face);
        }
      }
    }
//-----------------------------------------------------------------------------
// update seed time and X and Y coordinates, accurate knowledge of Z is not very relevant
//-----------------------------------------------------------------------------
    Seed->CalculateCogAndChi2(_sigmaR2);
  }

//-----------------------------------------------------------------------------
// loop over stations
// start with a seed
// move to next station
// loop over seeds, look for one with similar xy and time (split into components impractical?)
// if multiple, select one with best chi2
// continue, incrementing over stations until all finished
// what to do if there are gaps in the path?
// update center of mass?
//-----------------------------------------------------------------------------
  void DeltaFinder::connectSeeds() {

    for (int is=0; is<kNStations; is++) {
//-----------------------------------------------------------------------------
// 1. loop over existing compton seeds and match them to existing delta candidates
//-----------------------------------------------------------------------------
      int nseeds = _data.NComptonSeeds(is);
      for (int ids=0; ids<nseeds; ids++) {
        DeltaSeed* seed = _data.ComptonSeed(is,ids);

        if (! seed->Good() )                                          continue;
        if (seed->Used()   )                                          continue;
//-----------------------------------------------------------------------------
// first, loop over existing delta candidates and try to associate the seed
// with one of them
//-----------------------------------------------------------------------------
        DeltaCandidate* closest(nullptr);
        float           chi2min (_maxChi2SeedDelta);

        int ndelta = _data.nDeltaCandidates();
        for (int idc=0; idc<ndelta; idc++) {
          DeltaCandidate* dc = _data.deltaCandidate(idc);
//-----------------------------------------------------------------------------
// skip candidates already merged with others
//-----------------------------------------------------------------------------
          if (dc->Active() == 0      )                                continue;
          if (dc->seed[is] != nullptr)                                continue;
          int last = dc->LastStation();
//-----------------------------------------------------------------------------
// a delta candidate starting from a seed in a previous station may already have
// a seed in this station found, skip such a candidate
//-----------------------------------------------------------------------------
          if (last == is)                                             continue;
          int gap  = is-last;
          if (gap > _maxGap)                                          continue;
          float t0 = dc->T0(is);
          assert(t0 > 0);
          float dt = t0-seed->TMean();
          if (fabs(dt) > _maxSeedDt+_maxDtDs*gap)                     continue;
//-----------------------------------------------------------------------------
// the time is OK'ish - checks should be implemented more accurately (FIXME)
// look at the coordinates
//-----------------------------------------------------------------------------
          float chi2 = _finder->seedDeltaChi2(seed,dc);
          if (chi2 < chi2min) {
//-----------------------------------------------------------------------------
// everything matches, new closest delta candidate
//-----------------------------------------------------------------------------
            closest = dc;
            chi2min = chi2;
          }
        }
//-----------------------------------------------------------------------------
// if a DeltaSeed has been "attached" to a DeltaCandidate, this is it.
//-----------------------------------------------------------------------------
        if (closest) {
          closest->AddSeed(seed,is);
          seed->fChi2Delta = chi2min;
                                                                      continue;
        }
//-----------------------------------------------------------------------------
// DeltaSeed has not been linked to any existing delta candidate, create
// a new delta candidate and see if it is good enough
//-----------------------------------------------------------------------------
        DeltaCandidate delta(ndelta,seed,is);
//-----------------------------------------------------------------------------
// first, try to extend it backwards, in a compact way, to pick missing single hits
// seeds should've already been picked up !
//-----------------------------------------------------------------------------
        for (int is2=is-1; is2>=0; is2--) {
          recoverStation(&delta,delta.fFirstStation,is2,1,0);
//-----------------------------------------------------------------------------
// continue only if found something, allow one gap
//-----------------------------------------------------------------------------
          if (delta.fFirstStation-is2 > 2) break;
        }
//-----------------------------------------------------------------------------
// next, try to extend it forward, by one step, use of seeds is allowed
//-----------------------------------------------------------------------------
        if (is < kNStations-1) {
          recoverStation(&delta,is,is+1,1,1);
        }
//-----------------------------------------------------------------------------
// store only delta candidates with hits in more than 2 stations
// for each station define expected T0min and T0max
// to keep _minNSeeds=2 need to look forward...
// 'addDeltaCandidate' makes a deep copy
//-----------------------------------------------------------------------------
        if (delta.fNSeeds >= _minNSeeds) {
          _data.addDeltaCandidate(&delta);
        }
        else {
//-----------------------------------------------------------------------------
// mark all seeds as unassigned, this should be happening only in 1-seed case
//-----------------------------------------------------------------------------
          for (int is=delta.fFirstStation; is<=delta.fLastStation; is++) {
            DeltaSeed* ds = delta.seed[is];
            if (ds) ds->fDeltaIndex = -1;
          }
        }
      }
//-----------------------------------------------------------------------------
// all seeds in a given station processed
// loop over existing delta candidates which do not have seeds in a given station
// and see if can pick up single hits
//-----------------------------------------------------------------------------
      int ndelta = _data.nDeltaCandidates();
      for (int idc=0; idc<ndelta; idc++) {
        DeltaCandidate* dc = _data.deltaCandidate(idc);
        int last = dc->LastStation();
        if (last != is-1)                                           continue;
//-----------------------------------------------------------------------------
// if a delta candidate has been created in this routine, time limits
// may not be defined. Make sure they are
//-----------------------------------------------------------------------------
        recoverStation(dc,last,is,1,0);
      }
    }

//-----------------------------------------------------------------------------
// at this point we have a set of delta candidates, which might need to be merged
//-----------------------------------------------------------------------------
    mergeDeltaCandidates();
  }

//-----------------------------------------------------------------------------
  bool DeltaFinder::findData(const art::Event& Evt) {
    _data.chcol    = nullptr;
    _data.tpeakcol = nullptr;

    if (_useTimePeaks == 1){
      auto tpeakH = Evt.getValidHandle<mu2e::TimeClusterCollection>(_tpeakCollTag);
      _data.tpeakcol = tpeakH.product();
    }

    auto chcH   = Evt.getValidHandle<mu2e::ComboHitCollection>(_chCollTag);
    _data.chcol = chcH.product();

    auto shcH = Evt.getValidHandle<mu2e::StrawHitCollection>(_shCollTag);
    _shColl   = shcH.product();

    return (_data.chcol != nullptr) and (_shColl != nullptr);
  }

//-----------------------------------------------------------------------------
// find delta electron seeds in 'Station' with hits in faces 'f' and 'f+1'
// do not consider proton hits with eDep > _minHtEnergy
//-----------------------------------------------------------------------------
  void DeltaFinder::findSeeds(int Station, int Face) {

    for (int ip1=0; ip1<3; ++ip1) {                        // loop over panels in this face
      PanelZ_t* pz1 = &_data.oTracker[Station][Face][ip1];
      double nx1    = pz1->wx;
      double ny1    = pz1->wy;
      int    nh1    = pz1->fHitData->size();
      for (int h1=0; h1<nh1; ++h1) {
//-----------------------------------------------------------------------------
// hit has not been used yet to start a seed, however it could've been used as a second seed
//-----------------------------------------------------------------------------
        HitData_t*      hd1 = &(*pz1->fHitData)[h1];
        float           ct1 = hd1->fCorrTime;
        double x1           = hd1->fHit->pos().x();
        double y1           = hd1->fHit->pos().y();

        int counter         = 0;                // number of stereo candidate hits close to set up counter
//-----------------------------------------------------------------------------
// loop over the second faces
//-----------------------------------------------------------------------------
        for (int f2=Face+1; f2<kNFaces; f2++) {
          for (int ip2=0; ip2<3; ++ip2) {         // loop over panels
            PanelZ_t* pz2 = &_data.oTracker[Station][f2][ip2];
//-----------------------------------------------------------------------------
// check if the two panels overlap in XY
// 2D angle between the vectors pointing to the panel centers, can't be greater than 2*pi/3
//-----------------------------------------------------------------------------
            int iss = Station % 2;

            // bool c2 = _data.panelOverlap[iss][pz1->fID][pz2->fID] == 0;

            // if (c1 != c2) {
            //   printf("ERROR in DeltaFinder::%s: station:%2i ",__func__,Station);
            //   printf("Face,ip1,ID1,Z1,nx1,ny1,f2,ip2,ID2,Z2,nx2,ny2,c1,c2:");
            //   printf("%2i %2i %2i %8.2f %8.5f %8.5f %8.5f ",Face,ip1,pz1->fID,pz1->z,pz1->phi,pz1->nx,pz1->ny);
            //   printf("%2i %2i %2i %8.2f %8.5f %8.5f %8.5f ",f2  ,ip2,pz2->fID,pz2->z,pz2->phi,pz2->nx,pz2->ny);
            //   printf("%d %d\n",c1,c2);
            // }

            // if (fabs(dphi) >= 2*M_PI/3.)                              continue;
            if (_data.panelOverlap[iss][pz1->fID][pz2->fID] == 0)     continue;
//-----------------------------------------------------------------------------
// panels do overlap, check the time. tmin and tmax also detect panels w/o hits
//-----------------------------------------------------------------------------
            if (pz2->tmin - ct1 > _maxDriftTime)                      continue;
            if (ct1 - pz2->tmax > _maxDriftTime)                      continue;

            double nx2   = pz2->wx;
            double ny2   = pz2->wy;
            double n1n2  = nx1*nx2+ny1*ny2;
            double q12   = 1-n1n2*n1n2;
            double res_z = (pz1->z+pz2->z)/2;

            int    nh2 = pz2->fHitData->size();
            for (int h2=0; h2<nh2;++h2) {
              HitData_t* hd2 = &(*pz2->fHitData)[h2];
              float      ct2 = hd2->fCorrTime;
//-----------------------------------------------------------------------------
// hits are ordered in time, so if ct2-ct > _maxDriftTime, can proceed with the next panel
//-----------------------------------------------------------------------------
              if (ct2 - ct1 > _maxDriftTime)                           break;
              if (ct1 - ct2 > _maxDriftTime)                           continue;
              ++counter;                                            // number of hits close to the first one
//-----------------------------------------------------------------------------
// intersect the two straws, we need coordinates of the intersection point and
// two distances from hits to the intersection point, 4 numbers in total
//-----------------------------------------------------------------------------
              double x2    = hd2->fHit->pos().x();
              double y2    = hd2->fHit->pos().y();

              double r12n1 = (x1-x2)*nx1+(y1-y2)*ny1;
              double r12n2 = (x1-x2)*nx2+(y1-y2)*ny2;

              double wd1   = -(r12n2*n1n2-r12n1)/q12;

              double res_x = x1-nx1*wd1;
              double res_y = y1-ny1*wd1;

              double wd2   = -(r12n2-n1n2*r12n1)/q12;
//-----------------------------------------------------------------------------
// require both hits to be close enough to the intersection point
//-----------------------------------------------------------------------------
              float hd1_chi2 = wd1*wd1/hd1->fSigW2;
              float hd2_chi2 = wd2*wd2/hd2->fSigW2;
              if (hd1_chi2 > _maxChi2Seed)                            continue;
              if (hd2_chi2 > _maxChi2Seed)                            continue;
              if ((hd1_chi2+hd2_chi2)/2 > _maxChi2Seed)               continue;
//-----------------------------------------------------------------------------
// check whether there already is a seed containing both hits
//-----------------------------------------------------------------------------
              int is_duplicate  = checkDuplicates(Station,Face,hd1,f2,hd2);
              if (is_duplicate)                               continue;
//-----------------------------------------------------------------------------
// new seed : an intersection of two wires coresponsing to close in time combo hits
//-----------------------------------------------------------------------------
              hd1->fChi2Min     = hd1_chi2;
              hd2->fChi2Min     = hd2_chi2;

              DeltaSeed* seed   = _data.NewDeltaSeed(Station,Face,hd1,f2,hd2);

              seed->CofM.SetXYZ(res_x,res_y,res_z);
              // seed->fPhi = polyAtan2(res_y,res_x);
//-----------------------------------------------------------------------------
// mark both hits as a part of a seed, so they would not be used individually
// - see HitData_t::Used()
//-----------------------------------------------------------------------------
              hd1->fSeed  = seed;
              hd2->fSeed  = seed;
//-----------------------------------------------------------------------------
// complete search for hits of this seed, mark it BAD (or 'not-LEE') if a proton
// in principle, could place "high-charge" seeds into a separate list
// that should improve the performance
// if a seed EDep > _maxSeedEDep       (5 keV), can't be a low energy electron (LEE)
// if a seed EDep > _minProtonSeedEDep (3 keV), could be a proton
//-----------------------------------------------------------------------------
              completeSeed(seed);
              if (seed->EDep() > _maxSeedEDep) {
                seed->fGood = -2000-seed->fIndex;
              }
              else {
                _data.AddComptonSeed(seed,Station);
              }

              if (seed->EDep() > _minProtonSeedEDep) {
                _data.AddProtonSeed(seed,Station);
              }
            }
          }
        }
 //-----------------------------------------------------------------------------
 // this is needed for diagnostics only
 //-----------------------------------------------------------------------------
        // if (_diagLevel > 0) {
        //   hd1->fNSecondHits  = counter ;
        // }
      }
    }
  }

//-----------------------------------------------------------------------------
// TODO: update the time as more hits are added
//-----------------------------------------------------------------------------
  void DeltaFinder::findSeeds() {

    for (int s=0; s<kNStations; ++s) {
      for (int face=0; face<kNFaces-1; face++) {
        findSeeds(s,face);
      }
      pruneSeeds(s);
    }
  }

//-----------------------------------------------------------------------------
// define the time cluster parameters starting from a DeltaCandidate
//-----------------------------------------------------------------------------
  void DeltaFinder::initTimeCluster(DeltaCandidate* Dc, TimeCluster* Tc) {
  }

//-----------------------------------------------------------------------------
// merge Delta Candidates : check for duplicates !
//-----------------------------------------------------------------------------
  int DeltaFinder::mergeDeltaCandidates() {
    int rc(0);
    float max_d2(20*20);  // mm^2, to be adjusted FIXME

    int ndelta = _data.nDeltaCandidates();

    for (int i1=0; i1<ndelta-1; i1++) {
      DeltaCandidate* dc1 = _data.deltaCandidate(i1);
      if (dc1->Active() == 0)                                          continue;
      float x1 = dc1->CofM.x();
      float y1 = dc1->CofM.y();
      for (int i2=i1+1; i2<ndelta; i2++) {
        DeltaCandidate* dc2 = _data.deltaCandidate(i2);
        if (dc2->Active() == 0)                                        continue;
        float x2 = dc2->CofM.x();
        float y2 = dc2->CofM.y();
        float d2 = (x1-x2)*(x1-x2)+(y1-y2)*(y1-y2);
        if (d2 > max_d2)                                               continue;
//-----------------------------------------------------------------------------
// too lazy to extrapolate the time to the same Z...  ** FIXME
//-----------------------------------------------------------------------------
        float t1 = dc1->T0(dc1->LastStation());
        float t2 = dc2->T0(dc2->FirstStation());
//-----------------------------------------------------------------------------
// time check could be done more intelligently - compare time at the same Z
//-----------------------------------------------------------------------------
        if (fabs(t1-t2) > _maxDtDc)                                    continue;

        int dds = dc2->FirstStation()-dc1->LastStation();
        if (dds < 0) {
          if (_printErrors) {
            printf("ERROR in DeltaFinder::%s:",__func__);
            printf("i1, i2, dc1->LastStation, dc2->FirstStation: %2i %2i %2i %2i \n",
                   i1,i2,dc1->LastStation(),dc2->FirstStation());
          }
                                                                       continue;
        }
        else if (dds > _maxGap)                                        continue;
//-----------------------------------------------------------------------------
// merge two delta candidates, not too far from each other in Z,
// leave dc1 active, mark dc2 as not active
//-----------------------------------------------------------------------------
        dc1->MergeDeltaCandidate(dc2,_printErrors);
        dc2->SetIndex(-1000-dc1->Index());
      }
    }
    return rc;
  }

// //-----------------------------------------------------------------------------
// // Custom comparator to sort in ascending order
// //-----------------------------------------------------------------------------
//   bool comparator(const ComboHit*& a, const ComboHit*& b) {
//     return a->correctedTime() < b->correctedTime();
//   }

// //------------------------------------------------------------------------------
// // I'd love to use the hit flags, however that is confusing:
// // - hits with very large deltaT get placed to the middle of the wire and not flagged,
// // - however, some hits within the fiducial get flagged with the ::radsel flag...
// // use only "good" hits
// //-----------------------------------------------------------------------------
//   int DeltaFinder::orderHits() {
//     ChannelID cx, co;
// //-----------------------------------------------------------------------------
// // vector of pointers to CH, ordered in time. Initial list is not touched
// //-----------------------------------------------------------------------------
//     _data._v.resize(_data._nComboHits);

//     for (int i=0; i<_data._nComboHits; i++) {
//       _data._v[i] = &(*_data.chcol)[i];
//     }

//     std::sort(_data._v.begin(), _data._v.end(), comparator);
// //-----------------------------------------------------------------------------
// // at this point hits in '_data._v' are ordered in time
// //-----------------------------------------------------------------------------
//     for (int ih=0; ih<_data._nComboHits; ih++) {
//       const ComboHit* ch = _data._v[ih];

//       const StrawHitFlag* flag   = &ch->flag();
//       if (_testHitMask && (! flag->hasAllProperties(_goodHitMask) || flag->hasAnyProperty(_bkgHitMask)) ) continue;

//       float corr_time    = ch->correctedTime();

//       if ((corr_time      < _minT) || (corr_time > _maxT))  continue;

//       cx.Station                 = ch->strawId().station();
//       cx.Plane                   = ch->strawId().plane() % 2;
//       cx.Face                    = -1;
//       cx.Panel                   = ch->strawId().panel();

//                                               // get Z-ordered location
//       Data_t::orderID(&cx, &co);

//       int os       = co.Station;
//       int of       = co.Face;
//       int op       = co.Panel;

//       if (_useTimePeaks == 1) {
//         bool               intime(false);
//         int                nTPeaks  = _data.tpeakcol->size();
//         const CaloCluster* cl(nullptr);
//         int                iDisk(-1);

//         for (int i=0; i<nTPeaks; ++i) {
//           cl    = _data.tpeakcol->at(i).caloCluster().get();
//           if (cl == nullptr) {
//             printf(">>> DeltaFinder::orderHits() no CaloCluster found within the time peak %i\n", i);
//             continue;
//           }
//           iDisk = cl->diskID();
//           double    dt = cl->time() - (corr_time + _data.stationToCaloTOF[iDisk][os]);
//           if ( (dt < _maxCaloDt) && (dt > _minCaloDt) ) {
//             intime = true;
//             break;
//           }
//         }
//         if (!intime)                                    continue;
//       }

//       PanelZ_t* pz = &_data.oTracker[os][of][op];

//       if (_printErrors) {
//         if ((os < 0) || (os >= kNStations     )) printf(" >>> ERROR: wrong station number: %i\n",os);
//         if ((of < 0) || (of >= kNFaces        )) printf(" >>> ERROR: wrong face    number: %i\n",of);
//         if ((op < 0) || (op >= kNPanelsPerFace)) printf(" >>> ERROR: wrong panel   number: %i\n",op);
//       }

//       pz->fHitData->push_back(HitData_t(ch));
//       if (pz->tmin > corr_time) pz->tmin = corr_time;
//       if (pz->tmax < corr_time) pz->tmax = corr_time;
// //-----------------------------------------------------------------------------
// // prototype face-based hit storage
// // hits are already time-ordered - that makes it easy to define fFirst
// //-----------------------------------------------------------------------------
//       FaceZ_t* fz = &_data.fFaceData[os][of];
//       int loc = fz->fHitData.size();

//       fz->fHitData.push_back(HitData_t(ch));
//       int time_bin = int (corr_time/_timeBinWidth) ;

//       if (fz->fFirst[time_bin] < 0) fz->fFirst[time_bin] = loc;
//       fz->fLast[time_bin] = loc;
//     }

//     return 0;
//   }

//-----------------------------------------------------------------------------
  void DeltaFinder::produce(art::Event& Event) {
    if (_debugLevel) printf(">>> DeltaFinder::produce  event number: %10i\n",Event.event());
//-----------------------------------------------------------------------------
// clear memory in the beginning of event processing and cache event pointer
//-----------------------------------------------------------------------------
    _data.InitEvent(&Event,_debugLevel);
//-----------------------------------------------------------------------------
// process event
//-----------------------------------------------------------------------------
    if (! findData(Event)) {
      const char* message = "mu2e::DeltaFinder_module::produce: data missing or incomplete";
      throw cet::exception("RECO")<< message << endl;
    }

    _data._nComboHits = _data.chcol->size();
    _data._nStrawHits = _shColl->size();

    runDeltaFinder();
    // _finder->run();
//-----------------------------------------------------------------------------
// form output - flag combo hits -
// if flagged combo hits are written out, likely don't need writing out the flags
//-----------------------------------------------------------------------------
    unique_ptr<StrawHitFlagCollection> up_chfcol(new StrawHitFlagCollection(_data._nComboHits));
    _data.outputChfColl = up_chfcol.get();

    for (int i=0; i<_data._nComboHits; i++) {
      const ComboHit* ch = &(*_data.chcol)[i];
      (*_data.outputChfColl)[i].merge(ch->flag());
    }

    const ComboHit* ch0(0);
    if (_data._nComboHits > 0) ch0 = &_data.chcol->at(0);

    StrawHitFlag deltamask(StrawHitFlag::bkg);

    unique_ptr<TimeClusterCollection>  tcColl(new TimeClusterCollection);

    int ndeltas = _data.nDeltaCandidates();

    for (int i=0; i<ndeltas; i++) {
      DeltaCandidate* dc = _data.deltaCandidate(i);
//-----------------------------------------------------------------------------
// skip merged in delta candidates
// also require a delta candidate to have at least 5 hits
// do not consider proton stub candidates (those with <EDep> > 0.004)
//-----------------------------------------------------------------------------
      if (dc->Active() == 0)                                          continue;
      if (dc->NHits () < _minDeltaNHits)                              continue;
      if (dc->EDep  () > _maxDeltaEDep )                              continue;
      for (int station=dc->fFirstStation; station<=dc->fLastStation; station++) {
        DeltaSeed* ds = dc->seed[station];
        if (ds != nullptr) {
//-----------------------------------------------------------------------------
// loop over the hits and flag each of them as delta
//-----------------------------------------------------------------------------
          for (int face=0; face<kNFaces; face++) {
            const HitData_t* hd = ds->HitData(face);
            if (hd == nullptr)                                        continue;
            int loc = hd->fHit-ch0;
            _data.outputChfColl->at(loc).merge(deltamask);
          }
        }
      }
//-----------------------------------------------------------------------------
// make a time cluster out of each active DeltaCandidate
//-----------------------------------------------------------------------------
      TimeCluster new_tc;
      initTimeCluster(dc,&new_tc);
      tcColl->push_back(new_tc);
    }

    Event.put(std::move(tcColl));
//-----------------------------------------------------------------------------
// in the end of event processing fill diagnostic histograms
//-----------------------------------------------------------------------------
    if (_diagLevel  > 0) _hmanager->fillHistograms(&_data);
    if (_debugLevel > 0) _hmanager->debug(&_data,2);

    if (_writeComboHits) {
//-----------------------------------------------------------------------------
// write out collection of ComboHits with right flags, use deep copy
//-----------------------------------------------------------------------------
      auto outputChColl = std::make_unique<ComboHitCollection>();
      outputChColl->reserve(_data._nComboHits);

      outputChColl->setParent(_data.chcol->parent());
      for (int i=0; i<_data._nComboHits; i++) {
        StrawHitFlag const* flag = &(*_data.outputChfColl)[i];
        if (flag->hasAnyProperty(_bkgHitMask))                        continue;
//-----------------------------------------------------------------------------
// for the moment, assume bkgHitMask to be empty, so write out all hits
//-----------------------------------------------------------------------------
        const ComboHit* ch = &(*_data.chcol)[i];
        outputChColl->push_back(*ch);
        outputChColl->back()._flag.merge(*flag);
      }
      Event.put(std::move(outputChColl));
    }
//-----------------------------------------------------------------------------
// create the collection of StrawHitFlag for the StrawHitCollection
//-----------------------------------------------------------------------------
    if (_writeStrawHitFlags == 1) {
                                        // first, copy over the original flags

      std::unique_ptr<StrawHitFlagCollection> shfcol(new StrawHitFlagCollection(_data._nStrawHits));

      for(int ich=0; ich<_data._nComboHits; ich++) {
        const ComboHit* ch   = &(*_data.chcol )[ich];
        StrawHitFlag    flag =  (*_data.outputChfColl)[ich];
        flag.merge(ch->flag());
        for (auto ish : ch->indexArray()) {
          (*shfcol)[ish] = flag;
        }
      }

      Event.put(std::move(shfcol),"StrawHits");
    }
//-----------------------------------------------------------------------------
// moving in the end, after diagnostics plugin routines have been called - move
// invalidates the original pointer...
//-----------------------------------------------------------------------------
    Event.put(std::move(up_chfcol),"ComboHits");
  }

//-----------------------------------------------------------------------------
// some of found seeds could be duplicates or ghosts
// in case two DeltaSeeds share the first seed hit, leave only the best one
// the seeds we're loooping over have been reconstructed within the same station
// also reject seeds with Chi2Tot > _maxChi2Tot=10
//-----------------------------------------------------------------------------
  void DeltaFinder::pruneSeeds(int Station) {

    int nseeds =  _data.NSeeds(Station);

    for (int i1=0; i1<nseeds-1; i1++) {
      DeltaSeed* ds1 = _data.deltaSeed(Station,i1);
      if (ds1->fGood < 0)                                             continue;

      if (ds1->Chi2AllN() > _maxChi2All) {
        ds1->fGood = -1000-i1;
                                                                      continue;
      }

      float tmean1 = ds1->TMean();

      for (int i2=i1+1; i2<nseeds; i2++) {
        DeltaSeed* ds2 = _data.deltaSeed(Station,i2);
        if (ds2->fGood < 0)                                           continue;

        if (ds2->Chi2AllN() > _maxChi2All) {
          ds2->fGood = -1000-i2;
                                                                      continue;
        }

        float tmean2 = ds2->TMean();

        if (fabs(tmean1-tmean2) > _maxSeedDt)                         continue;
//-----------------------------------------------------------------------------
// the two segments are close in time , both have acceptable chi2's
// *FIXME* didn't check distance !!!!!
// so far, allow duplicates during the search
// the two DeltaSeeds share could have significantly overlapping hit content
//-----------------------------------------------------------------------------
        int noverlap            = 0;
        int nfaces_with_overlap = 0;
        for (int face=0; face<kNFaces; face++) {
          const HitData_t* hh1 = ds1->hitData[face];
          const HitData_t* hh2 = ds2->hitData[face];
          if (hh1 and (hh1 == hh2)) {
            noverlap            += 1;
            nfaces_with_overlap += 1;
          }
        }

        if (nfaces_with_overlap > 1) {
//-----------------------------------------------------------------------------
// overlap significant, leave in only one DeltaSeed - which one?
//-----------------------------------------------------------------------------
          if (ds1->fNFacesWithHits > ds2->fNFacesWithHits) {
            ds2->fGood = -1000-i1;
          }
          else if (ds2->fNFacesWithHits > ds1->fNFacesWithHits) {
            ds1->fGood = -1000-i2;
            break;
          }
          else {
//-----------------------------------------------------------------------------
//both seeds have the same number of hits - compare chi2's
//-----------------------------------------------------------------------------
            if (ds1->Chi2AllN() <  ds2->Chi2AllN()) {
              ds2->fGood = -1000-i1;
            }
            else {
              ds1->fGood = -1000-i2;
              break;
            }
          }
        }
        else if (nfaces_with_overlap > 0) {
//-----------------------------------------------------------------------------
// only one overlapping hit
// special treatment of 2-hit seeds to reduce the number of ghosts
//-----------------------------------------------------------------------------
          if (ds1->fNFacesWithHits == 2) {
            if (ds2->fNFacesWithHits > 2) {
              ds1->fGood = -1000-i2;
              break;
            }
            else {
//-----------------------------------------------------------------------------
// the second seed also has 2 faces with hits
//-----------------------------------------------------------------------------
              if (ds1->Chi2AllN() <  ds2->Chi2AllN()) ds2->fGood = -1000-i1;
              else {
                ds1->fGood = -1000-i2;
                break;
              }
            }
          }
          else {
//-----------------------------------------------------------------------------
// the first seed has N>2 hits
//-----------------------------------------------------------------------------
            if (ds2->fNFacesWithHits == 2) {
//-----------------------------------------------------------------------------
// the 2nd seed has only 2 hits and there is an overlap
//-----------------------------------------------------------------------------
              ds2->fGood = -1000-i1;
            }
            else {
//-----------------------------------------------------------------------------
// the second seed also has N>2 faces with hits, but there is only one overlap
// leave both seeds in
//-----------------------------------------------------------------------------
            }
          }
        }
      }
    }
  }

//------------------------------------------------------------------------------
// start from looking at the "holes" in the seed pattern
// delta candidates in the list are already required to have at least 2 segments
// extend them outwards by one station
//-----------------------------------------------------------------------------
  int DeltaFinder::recoverMissingHits() {

    int ndelta = _data.nDeltaCandidates();
    for (int idelta=0; idelta<ndelta; idelta++) {
      DeltaCandidate* dc = _data.deltaCandidate(idelta);
//-----------------------------------------------------------------------------
// don't extend candidates made out of one segment - but there is no such
// start from the first station to define limits
//-----------------------------------------------------------------------------
      int s1 = dc->fFirstStation;
      int s2 = dc->fLastStation-1;
      int last(-1);
//-----------------------------------------------------------------------------
// first check inside "holes"
//-----------------------------------------------------------------------------
      for (int i=s1; i<=s2; i++) {
        if (dc->seed[i] != nullptr) {
          last  = i;
          continue;
        }
//-----------------------------------------------------------------------------
// define expected T0 limits
//-----------------------------------------------------------------------------
        recoverStation(dc,last,i,1,0);
      }

      last  = dc->fFirstStation;
      for (int i=last-1; i>=0; i--) {
//-----------------------------------------------------------------------------
// skip empty stations
//-----------------------------------------------------------------------------
        if (dc->fFirstStation -i > _maxGap) break;
        recoverStation(dc,dc->fFirstStation,i,1,0);
      }

      last = dc->fLastStation;
      for (int i=last+1; i<kNStations; i++) {
//-----------------------------------------------------------------------------
// skip empty stations
//-----------------------------------------------------------------------------
        if (i-dc->fLastStation > _maxGap) break;
        recoverStation(dc,dc->fLastStation,i,1,0);
      }
    }

    return 0;
  }

// //-----------------------------------------------------------------------------
// // *FIXME* : need a formula for chi2, this simplification may not work well
// // in the corners
// //-----------------------------------------------------------------------------
//   double DeltaFinder::seedDeltaChi2(DeltaSeed* Seed, DeltaCandidate* Delta) {

//     int    nh   = Delta->NHits()+Seed->NHits();

//     double nxym = (Delta->fSnxy+Seed->fSnxy)/nh;
//     double nx2m = (Delta->fSnx2+Seed->fSnx2)/nh;
//     double ny2m = (Delta->fSny2+Seed->fSny2)/nh;
//     double nxrm = (Delta->fSnxr+Seed->fSnxr)/nh;
//     double nyrm = (Delta->fSnyr+Seed->fSnyr)/nh;

//     double d    = nx2m*ny2m-nxym*nxym;
//     double xc   = (nyrm*nx2m-nxrm*nxym)/d;
//     double yc   = (nyrm*nxym-nxrm*ny2m)/d;

//     double dxs  = xc-Seed->Xc();
//     double dys  = yc-Seed->Yc();

//     double dxd  = xc-Delta->Xc();
//     double dyd  = yc-Delta->Yc();

//     double chi2 = (dxs*dxs+dys*dys+dxd*dxd+dyd*dyd)/(_maxDxy*_maxDxy);

//     return chi2;
//   }


//-----------------------------------------------------------------------------
// return 1 if a seed has been found , 0 otherwise
//-----------------------------------------------------------------------------
  int DeltaFinder::recoverSeed(DeltaCandidate* Delta, int LastStation, int Station) {
    // int rc(0);
                                        // predicted time range for this station
    float tdelta = Delta->T0(Station);

    float      chi2min(_maxChi2SeedDelta);
    DeltaSeed* closest_seed(nullptr);

    float dt   = _maxSeedDt + _maxDtDs*fabs(Station-LastStation);

    int nseeds = _data.NComptonSeeds(Station);
    for (int i=0; i<nseeds; i++) {
      DeltaSeed* seed =  _data.ComptonSeed(Station,i);
      if (seed->Good() == 0)                                          continue;
      if (seed->Used()     )                                          continue;
//-----------------------------------------------------------------------------
// one might need some safety here, but not the _maxDriftTime
//-----------------------------------------------------------------------------
      if (fabs(tdelta-seed->TMean()) > dt)                            continue;

      float chi2 = _finder->seedDeltaChi2(seed,Delta);

      if (chi2 < chi2min) {
                                        // new best seed
        closest_seed = seed;
        chi2min      = chi2;
      }
    }

    if (closest_seed) {
//-----------------------------------------------------------------------------
// the closest seed found, add it to the delta candidate and exit
// it is marked as associated with the delta candidate in DeltaCandidate::AddSeed
//-----------------------------------------------------------------------------
      Delta->AddSeed(closest_seed,Station);
      closest_seed->fChi2Delta = chi2min;
    }

    return (closest_seed != nullptr);
  }

//------------------------------------------------------------------------------
// try to recover hits of a 'Delta' candidate in a given 'Station'
// 'Delta' doesn't have hits in this station, check all hits here
// when predicting time, use the same value of Z for both layers of a given face
// return 1 if something has been found
//-----------------------------------------------------------------------------
  int DeltaFinder::recoverStation(DeltaCandidate* Delta, int LastStation, int Station, int UseUsedHits, int RecoverSeeds) {

                                        // predicted time range for this station
    float tdelta = Delta->T0(Station);
    float xdelta = Delta->Xc();
    float ydelta = Delta->Yc();
    float rdelta = sqrt(xdelta*xdelta+ydelta*ydelta);
    float delta_nx = xdelta/rdelta;
    float delta_ny = ydelta/rdelta;
//-----------------------------------------------------------------------------
// first, loop over the existing seeds - need for the forward step
//-----------------------------------------------------------------------------
    if (RecoverSeeds) {
      int closest_seed_found = recoverSeed(Delta,LastStation,Station);
      if (closest_seed_found == 1) return 1;
    }
//-----------------------------------------------------------------------------
// no seeds found, look for single hits in the 'Station'
//-----------------------------------------------------------------------------
    DeltaSeed*  new_seed (nullptr);

    float dt_hit = _maxHitDt+_maxDtDs*fabs(Station-LastStation);

    for (int face=0; face<kNFaces; face++) {
      for (int ip=0; ip<kNPanelsPerFace; ip++) {
        PanelZ_t* pz = &_data.oTracker[Station][face][ip];
        float n1n2   = delta_nx*pz->nx+delta_ny*pz->ny;
//-----------------------------------------------------------------------------
// 0.5 corresponds to delta(phi) = 60 deg
//-----------------------------------------------------------------------------
        if (n1n2 < 0.5)                                               continue;
        if (tdelta-dt_hit > pz->tmax)                                 continue;
        if (tdelta+dt_hit < pz->tmin)                                 continue;
//-----------------------------------------------------------------------------
// panel and Delta overlap in phi and time, loop over hits
//-----------------------------------------------------------------------------
        int nhits = pz->fHitData->size();
        for (int h=0; h<nhits; ++h) {
          HitData_t* hd = &(*pz->fHitData)[h];
//-----------------------------------------------------------------------------
// don't skip hits already included into seeds - a two-hit stereo seed
// could be random
//-----------------------------------------------------------------------------
          if ((UseUsedHits == 0) and hd->Used())                      continue;
          float corr_time     = hd->fCorrTime;
//-----------------------------------------------------------------------------
// predicted time is the particle time, the drift time should be larger
//-----------------------------------------------------------------------------
          if (corr_time > tdelta+dt_hit)                              break;
          if (corr_time < tdelta-dt_hit)                              continue;

          const ComboHit*  ch = hd->fHit;
          double dx  = ch->pos().x()-xdelta;
          double dy  = ch->pos().y()-ydelta;
          double dw  = dx*pz->wx+dy*pz->wy; // distance along the wire
          double dxx = dx-pz->wx*dw;
          double dyy = dy-pz->wy*dw;

          double chi2_par  = (dw*dw)/(hd->fSigW2+_seedRes*_seedRes);
          double chi2_perp = (dxx*dxx+dyy*dyy)/_sigmaR2;
          double chi2      = chi2_par + chi2_perp;

          if (chi2 >= _maxChi2Radial)                                 continue;
          if (hd->Used()) {
//-----------------------------------------------------------------------------
// hit is a part of a seed. if the seed has 2 or less hits, don't check the chi2
// - that could be a random overlap
// if the seed has 3 or more hits, check the chi2
//-----------------------------------------------------------------------------
            int nh = hd->fSeed->NHits();
            if ((nh >= 3) and (chi2 > hd->fChi2Min))                   continue;
          }
//-----------------------------------------------------------------------------
// new hit needs to be added, create a special 1-hit seed for that
// in most cases, expect this seed not to have the second hit, but it may
// such a seed has its own CofM undefined
// ** FIXME ..in principle, at this point may want to check if the hit was used...
//-----------------------------------------------------------------------------
          if (new_seed == nullptr) {
            hd->fChi2Min = chi2;
            new_seed = _data.NewDeltaSeed(Station,face,hd,-1,nullptr);
          }
          else {
            if (face == new_seed->SFace(0)) {
//-----------------------------------------------------------------------------
// another close hit in the same panel, choose the best
//-----------------------------------------------------------------------------
              if (chi2 >= new_seed->HitData(face)->fChi2Min)          continue;
//-----------------------------------------------------------------------------
// new best hit in the same face
//-----------------------------------------------------------------------------
              hd->fChi2Min = chi2;
              new_seed->ReplaceFirstHit(hd);
            }
            else {
//-----------------------------------------------------------------------------
// more than one hit added in the hit pickup mode. The question is why a seed,
// constructed out of those two hits has not been added (or created)
//-----------------------------------------------------------------------------
              if (_printErrors) {
                printf("ERROR in DeltaFinder::recoverStation: ");
                printf("station=%2i - shouldn\'t be getting here, printout of new_seed and hd follows\n",Station);
                printf("chi2_par, chi2_perp, chi2: %8.2f %8.2f %8.2f\n",chi2_par, chi2_perp, chi2);

                printf("DELTA:\n");
                _data.printDeltaCandidate(Delta,"");
                printf("SEED:\n");
                _data.printDeltaSeed(new_seed,"");
                printf("HIT:\n");
                _data.printHitData  (hd      ,"");
              }

              new_seed->AddHit(hd,face);
            }

            if (corr_time < new_seed->fMinHitTime) new_seed->fMinHitTime = corr_time;
            if (corr_time > new_seed->fMaxHitTime) new_seed->fMaxHitTime = corr_time;
          }
        }
      }
      if (new_seed) new_seed->fFaceProcessed[face] = 1;
    }
//-----------------------------------------------------------------------------
// station is processed, see if anything has been found
// some parameters of seeds found in a recovery mode are not defined because
// there was no pre-seeding, for example
//-----------------------------------------------------------------------------
    int rc(0);
    if (new_seed) {
      int face0                       = new_seed->SFace(0);
      new_seed->HitData(face0)->fSeed = new_seed;

      Delta->AddSeed(new_seed,Station);
      rc = 1;
    }
                                        // return 1 if hits were added
    return rc;
  }

//-----------------------------------------------------------------------------
  void  DeltaFinder::runDeltaFinder() {

    _finder->orderHits();
//-----------------------------------------------------------------------------
// loop over all stations and find delta seeds - 2-3-4 combo hit stubs
// a seed is always a stereo object
//-----------------------------------------------------------------------------
    findSeeds();
//-----------------------------------------------------------------------------
// connect seeds and create delta candidates
// at this stage, extend seeds to pick up single its in neighbor stations
// for single hits do not allo gaps
//-----------------------------------------------------------------------------
    connectSeeds();
//-----------------------------------------------------------------------------
// for existing delta candidates, pick up single gap-separated single hits
// no new candidates is created at this step
//-----------------------------------------------------------------------------
    recoverMissingHits();
//-----------------------------------------------------------------------------
// after recovery of missing hits, it is possible that some of delta candidates
// may need to be merged - try again
//-----------------------------------------------------------------------------
    mergeDeltaCandidates();
  }
}

//-----------------------------------------------------------------------------
// magic that makes this class a module.
//-----------------------------------------------------------------------------
DEFINE_ART_MODULE(mu2e::DeltaFinder)
//-----------------------------------------------------------------------------
// done
//-----------------------------------------------------------------------------

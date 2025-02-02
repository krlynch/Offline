//
// Low-energy electron background diagnostics.  Split out of FlagBkgHits
//
// Original author D. Brown
//
//
/// framework
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "art/Framework/Core/EDAnalyzer.h"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "art_root_io/TFileService.h"
// root
#include "TMath.h"
#include "TH1F.h"
#include "TTree.h"
#include "Math/VectorUtil.h"
// data
#include "Offline/RecoDataProducts/inc/StrawHit.hh"
#include "Offline/RecoDataProducts/inc/ComboHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHitFlag.hh"
#include "Offline/RecoDataProducts/inc/BkgCluster.hh"
#include "Offline/RecoDataProducts/inc/BkgClusterHit.hh"
#include "Offline/RecoDataProducts/inc/BkgQual.hh"
#include "Offline/MCDataProducts/inc/StrawDigiMC.hh"
#include "Offline/MCDataProducts/inc/MCRelationship.hh"
#include "Offline/MCDataProducts/inc/SimParticle.hh"
#include "Offline/DataProducts/inc/PDGCode.hh"
#include "Offline/TrkDiag/inc/BkgHitInfo.hh"
// art
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Sequence.h"
using std::string;
using namespace ROOT::Math::VectorUtil;
namespace mu2e
{

  class BkgDiag : public art::EDAnalyzer {
    public:

      struct Config {
        using Name = fhicl::Name;
        using Comment = fhicl::Comment;
        fhicl::Atom<int> diag{ Name("diagLevel"), Comment("Diag Level"),0 };
        fhicl::Atom<int> debug{ Name("debugLevel"), Comment("Debug Level"),0 };
        fhicl::Atom<bool> mcdiag{ Name("MCDiag"), Comment("MonteCarlo Diag"), true };
        fhicl::Atom<bool> useflagcol{ Name("useFlagCollection"), Comment("Use Flag Collection"), false };
        fhicl::Atom<float> maxdt{ Name("maxTimeDifference"), Comment("Max Time Difference"), 50.0 };
        fhicl::Atom<float> maxdrho{ Name("maxRhoDifference"), Comment("Max Rho Difference"), 50.0 };
        fhicl::Atom<art::InputTag> ComboHitCollection{   Name("ComboHitCollection"),   Comment("ComboHit collection name") };
        fhicl::Atom<art::InputTag> StrawHitFlagCollection{   Name("StrawHitFlagCollection"),   Comment("StrawHitFlag collection name") };
        fhicl::Atom<art::InputTag> BkgClusterCollection{   Name("BkgClusterCollection"),   Comment("BackgroundCluster collection name") };
        fhicl::Atom<art::InputTag> BkgQualCollection{   Name("BkgQualCollection"),   Comment("BackgroundQual collection name") };
        fhicl::Atom<art::InputTag> BkgClusterHitCollection{   Name("BkgClusterHitCollection"),   Comment("BackgroundClusterHit collection name") };
        fhicl::Atom<art::InputTag> StrawDigiMCCollection{   Name("StrawDigiMCCollection"),   Comment("StrawDigiMC collection name") };
      };

      explicit BkgDiag(const art::EDAnalyzer::Table<Config>& config);
      virtual ~BkgDiag();
      virtual void beginJob();
      virtual void analyze(const art::Event& e);
    private:
      // helper functions
      void fillStrawHitInfo(size_t ich, StrawHitInfo& bkghinfo) const;
      void fillStrawHitInfoMC(StrawDigiMC const& mcdigi, art::Ptr<SimParticle>const& pptr, StrawHitInfo& shinfo) const;
      bool findData(const art::Event& e);
      void findPrimary(std::vector<StrawDigiIndex>const& dids, art::Ptr<SimParticle>& pptr,double& pmom, std::vector<int>& icontrib) const;

      // control flags
      int _diag,_debug;
      bool _mcdiag, _useflagcol;
      float _maxdt, _maxdrho;
      // data tags
      art::ProductToken<ComboHitCollection> _chToken;
      art::ProductToken<StrawHitFlagCollection> _shfToken;
      art::ProductToken<BkgClusterCollection> _bkgcToken;
      art::ProductToken<BkgQualCollection> _bkgqToken;
      art::ProductToken<BkgClusterHitCollection> _bkghToken;
      art::ProductToken<StrawDigiMCCollection> _mcdigisToken;
      // time offset
      // cache of event objects
      const ComboHitCollection* _chcol;
      const StrawHitFlagCollection* _shfcol;
      const StrawDigiMCCollection *_mcdigis;
      const BkgClusterCollection *_bkgccol;
      const BkgQualCollection *_bkgqcol;
      const BkgClusterHitCollection *_bkghitcol;

      // background diagnostics
      TTree* _bcdiag,*_bhdiag;
      int _iev = 0;
      XYZVectorF _cpos;
      float _ctime = 0;
      float _mindt = 0;
      float _mindrho = 0;
      bool _isbkg = false;
      bool _isref = false;
      bool _isolated = false;
      bool _stereo = false;
      int _cluIdx, _nactive, _nchits, _nshits, _nstereo, _nsactive, _nbkg;
      // BkgQual vars
      float _bkgqualvars[BkgQual::n_vars];
      int _mvastat;
      float _mvaout;

      // MC truth variables
      int _ppid, _ppdg, _pgen, _pproc, _ncontrib, _icontrib[512];
      float _pmom;
      int _nconv, _ndelta, _ncompt, _ngconv, _nebkg, _nprot, _nprimary;
      std::vector<BkgHitInfo> _bkghinfo;

      int   _nindex,_hitidx[8192];
      int   _nhits,_hitPdg[8192],_hitproc[8192],_hitNcombo[8192];
      std::vector<XYZVectorF> _hitPos;
      float _hitTime[8192];
  };

  BkgDiag::BkgDiag(const art::EDAnalyzer::Table<Config>& config) :
    art::EDAnalyzer{config},
    _diag( config().diag() ),
    _debug( config().debug() ),
    _mcdiag( config().mcdiag() ),
    _useflagcol( config().useflagcol() ),
    _maxdt( config().maxdt() ),
    _maxdrho( config().maxdrho() ),
    _chToken{ consumes<ComboHitCollection>(config().ComboHitCollection() ) },
    _shfToken{ consumes<StrawHitFlagCollection>(config().StrawHitFlagCollection() ) },
    _bkgcToken{ consumes<BkgClusterCollection>(config().BkgClusterCollection() ) },
    _bkgqToken{ consumes<BkgQualCollection>(config().BkgQualCollection() ) },
    _bkghToken{ consumes<BkgClusterHitCollection>(config().BkgClusterHitCollection() ) },
    _mcdigisToken{ consumes<StrawDigiMCCollection>(config().StrawDigiMCCollection() ) }
  {}

  BkgDiag::~BkgDiag(){}

  void BkgDiag::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;

    _iev=0;
    // detailed delta diagnostics
    _bcdiag=tfs->make<TTree>("bkgcdiag","background cluster diagnostics");
    // general branches
    _bcdiag->Branch("iev",&_iev,"iev/I");
    // cluster info branches
    _bcdiag->Branch("cpos",&_cpos);
    _bcdiag->Branch("ctime",&_ctime,"ctime/F");
    _bcdiag->Branch("isbkg",&_isbkg,"isbkg/B");
    _bcdiag->Branch("isref",&_isref,"isref/B");
    _bcdiag->Branch("isolated",&_isolated,"isolated/B");
    _bcdiag->Branch("stereo",&_stereo,"stereo/B");
    _bcdiag->Branch("mindt",&_mindt,"mindt/F");
    _bcdiag->Branch("mindrho",&_mindrho,"mindrho/F");
    _bcdiag->Branch("nchits",&_nchits,"nchits/I");
    _bcdiag->Branch("nshits",&_nshits,"nshits/I");
    _bcdiag->Branch("nactive",&_nactive,"nactive/I");
    _bcdiag->Branch("nstereo",&_nstereo,"nstereo/I");
    _bcdiag->Branch("nsactive",&_nsactive,"nsactive/I");
    _bcdiag->Branch("nbkg",&_nbkg,"nbkg/I");
    _bcdiag->Branch("cluIdx",&_cluIdx,"cluIdx/I");
    _bcdiag->Branch("nindex",&_nindex,"nindex/I");
    _bcdiag->Branch("hitidx",&_hitidx,"hitidx[nindex]/I");
    // cluster hit info branch
    if(_diag > 0)
      _bcdiag->Branch("bkghinfo",&_bkghinfo);
    // Bkg qual info
    for(int ivar=0;ivar < BkgQual::n_vars; ++ivar){
      string vname = BkgQual::varName(static_cast<BkgQual::MVA_varindex>(ivar));
      string bname = vname+string("/F");
      _bcdiag->Branch(vname.c_str(),&_bkgqualvars[ivar],bname.c_str());
    }
    _bcdiag->Branch("mvaout", &_mvaout,"mvaout/F");
    _bcdiag->Branch("mvastat", &_mvastat,"mvastat/I");
    // mc truth branches
    if(_mcdiag){
      _bcdiag->Branch("pmom",&_pmom,"pmom/F");
      _bcdiag->Branch("ppid",&_ppid,"ppid/I");
      _bcdiag->Branch("ppdg",&_ppdg,"ppdg/I");
      _bcdiag->Branch("pgen",&_pgen,"pgen/I");
      _bcdiag->Branch("pproc",&_pproc,"pproc/I");
      _bcdiag->Branch("nprimary",&_nprimary,"nprimary/I");
      _bcdiag->Branch("nconv",&_nconv,"nconv/I");
      _bcdiag->Branch("ndelta",&_ndelta,"ndelta/I");
      _bcdiag->Branch("ncompt",&_ncompt,"ncompt/I");
      _bcdiag->Branch("ngconv",&_ngconv,"ngconv/I");
      _bcdiag->Branch("nebkg",&_nebkg,"nebkg/I");
      _bcdiag->Branch("nprot",&_nprot,"nprot/I");
      _bcdiag->Branch("ncontrib",&_ncontrib,"ncontrib/I");
      _bcdiag->Branch("icontrib",&_icontrib,"icontrib[ncontrib]/I");
    }
    _bhdiag = tfs->make<TTree>("bkghdiag","background hit diagnostics");
    _bhdiag->Branch("iev",        &_iev,          "iev/I");
    _bhdiag->Branch("nhits",      &_nhits,        "nhits/I");
    _bhdiag->Branch("pos",     &_hitPos);
    _bhdiag->Branch("time",    &_hitTime,      "hitTime[nhits]/F");
    _bhdiag->Branch("ncombo",  &_hitNcombo,    "hitNcombo[nhits]/I");
    if(_mcdiag){
      _bhdiag->Branch("mcpdg",   &_hitPdg,       "hitPdg[nhits]/I");
      _bhdiag->Branch("mcproc",&_hitproc,    "hitproc[nhits]/I");
    }
  }

  void BkgDiag::analyze(const art::Event& event ) {
    if(!findData(event))
      throw cet::exception("RECO")<<"mu2e::BkgDiag: data missing or incomplete"<< std::endl;
    // check consistency
    if(_bkgccol->size() != _bkgqcol->size())
      throw cet::exception("RECO")<<"mu2e::BkgDiag: data inconsistent"<< std::endl;
    // loop over background clusters

    _nhits=0;
    _hitPos.clear();
    _hitPos.reserve(_chcol->size());
    for(size_t ich=0;ich<_chcol->size();++ich){
      _hitPos.push_back(_chcol->at(ich).pos());
      _hitTime[_nhits]   = _chcol->at(ich).time();
      _hitNcombo[_nhits] = _chcol->at(ich).nCombo();
      if(_mcdiag){
        std::vector<StrawDigiIndex> dids;
        _chcol->fillStrawDigiIndices(event,ich,dids);
        StrawDigiMC const& mcdigi = _mcdigis->at(dids[0]);// taking 1st digi: is there a better idea??
        art::Ptr<SimParticle> const& spp = mcdigi.earlyStrawGasStep()->simParticle();
        _hitPdg[_nhits] = spp->pdgId();
        _hitproc[_nhits] = spp->creationCode();
      }
      ++_nhits;
    }
    _bhdiag->Fill();

    _cluIdx=0;
    for (size_t ibkg=0;ibkg<_bkgccol->size();++ibkg){
      BkgCluster const& cluster = _bkgccol->at(ibkg);
      BkgQual const& qual = _bkgqcol->at(ibkg);
      // fill cluster info
      _cpos = cluster.pos();
      _ctime = cluster.time();
      _isbkg = cluster.flag().hasAllProperties(BkgClusterFlag::bkg);
      _isref = cluster.flag().hasAllProperties(BkgClusterFlag::refined);
      _isolated = cluster.flag().hasAllProperties(BkgClusterFlag::iso);
      _stereo = cluster.flag().hasAllProperties(BkgClusterFlag::stereo);
      // fill Bkg qual info
      for(int ivar=0;ivar < BkgQual::n_vars; ++ivar){
        _bkgqualvars[ivar] = qual[static_cast<BkgQual::MVA_varindex>(ivar)];
      }
      _mvaout = qual.MVAOutput();
      _mvastat = qual.status();
      // info on nearest cluster
      _mindt = _mindrho = 1.0e3;
      for(size_t jbkg = 0; jbkg < _bkgccol->size(); ++jbkg){
        if(ibkg != jbkg){
          BkgCluster const& ocluster = _bkgccol->at(jbkg);
          double dt = fabs(ocluster.time() - cluster.time());
          double drho = sqrt((ocluster.pos()-cluster.pos()).Perp2());
          // only look at differences whtn the other dimension difference is small
          if(drho < _maxdrho && dt < _mindt) _mindt = dt;
          if(dt < _maxdt && drho < _mindrho) _mindrho = drho;
        }
      }
      // fill mc info
      art::Ptr<SimParticle> pptr;
      // loop over hits in this cluster and classify them
      _nconv = 0;
      _nprot = 0;
      _ndelta= 0;
      _ncompt = 0;
      _ngconv = 0;
      _nebkg = 0;
      _nprimary = 0;
      _pmom = 0.0;
      _ppid = _ppdg = _pgen = _pproc = 0;
      _ncontrib = 0;
      if(_mcdiag){
        // fill vector of indices to all digis used in this cluster's hits
        // this goes recursively through the ComboHit chain
        std::vector<StrawDigiIndex> cdids;
        for(auto const& ich : cluster.hits()){
          // get the list of StrawHit indices associated with this ComboHit
          _chcol->fillStrawDigiIndices(event,ich,cdids);
        }
        double pmom(0.0);
        std::vector<int> icontrib;
        findPrimary(cdids,pptr,pmom,icontrib);
        for (int ic : icontrib) {_icontrib[_ncontrib]=ic; ++_ncontrib;}
        _pmom = pmom;
        if(pptr.isNonnull()){
          _ppid = pptr->id().asInt();
          _ppdg = pptr->pdgId();
          _pproc = pptr->creationCode();
          if( pptr->genParticle().isNonnull())
            _pgen = pptr->genParticle()->generatorId().id();
        }
      }
      // fill cluster hit info
      _bkghinfo.clear();
      _bkghinfo.reserve(cluster.hits().size());
      _nchits = cluster.hits().size();
      _nshits = 0;
      _nactive = _nstereo = _nsactive = _nbkg = 0;
      bool pce = _pgen==2; // primary from a CE
      _nindex=0;
      for(auto const& ich : cluster.hits()){
        ComboHit const& ch = _chcol->at(ich);
        BkgClusterHit const& bhit = _bkghitcol->at(ich);
        _hitidx[_nindex]=ich;
        ++_nindex;
        _nshits += ch.nStrawHits();
        StrawHitFlag const& shf = bhit.flag();
        if(shf.hasAllProperties(StrawHitFlag::active)){
          _nactive += ch.nStrawHits();
          if(shf.hasAllProperties(StrawHitFlag::stereo))_nsactive+= ch.nStrawHits();
        }
        if(shf.hasAllProperties(StrawHitFlag::stereo))_nstereo+= ch.nStrawHits();
        if(shf.hasAllProperties(StrawHitFlag::bkg))_nbkg+= ch.nStrawHits();
        // fill hit-specific information
        BkgHitInfo bkghinfo;
        // fill basic straw hit info
        fillStrawHitInfo(ich,bkghinfo);
        if(_mcdiag){
          std::vector<StrawDigiIndex> dids;
          _chcol->fillStrawDigiIndices(event,ich,dids);
          StrawDigiMC const& mcdigi = _mcdigis->at(dids[0]);// taking 1st digi: is there a better idea??
          fillStrawHitInfoMC(mcdigi,pptr,bkghinfo);
        }
        // background hit specific information
        bkghinfo._active = shf.hasAllProperties(StrawHitFlag::active);
        bkghinfo._cbkg = shf.hasAllProperties(StrawHitFlag::bkg);
        bkghinfo._gdist = bhit.distance();
        bkghinfo._index = ich;
        // calculate separation to cluster
        auto psep = ch.pos()-cluster.pos();
        auto pdir = PerpVector(psep,GenVector::ZDir()).Unit();
        bkghinfo._rpos = psep;
        bkghinfo._rerr = std::max(float(2.5),ch.posRes(ComboHit::wire)*fabs(pdir.Dot(ch.wdir())));
        //global counting for the cluster: count signal hits only, but background from background is OK
        if(pce){
          if(bkghinfo._relation==0) _nprimary += ch.nStrawHits(); // couunt only true primary
        } else {
          if(bkghinfo._relation>=0 && bkghinfo._relation <=3) _nprimary += ch.nStrawHits(); // count primar + mother/daughter/sibling
        }
        if(bkghinfo._mcgen == 2)_nconv += ch.nStrawHits();
        if(abs(bkghinfo._mcpdg) == PDGCode::e_minus && bkghinfo._mcgen <0){
          _nebkg += ch.nStrawHits();
          if(bkghinfo._mcproc == ProcessCode::eIoni ||bkghinfo._mcproc == ProcessCode::hIoni ){
            _ndelta += ch.nStrawHits();
          } else if(bkghinfo._mcproc == ProcessCode::compt){
            _ncompt += ch.nStrawHits();
          } else if(bkghinfo._mcproc == ProcessCode::conv){
            _ngconv += ch.nStrawHits();
          }
        }
        if(bkghinfo._mcpdg == PDGCode::proton)_nprot += ch.nStrawHits();
        _bkghinfo.push_back(bkghinfo);
      }
      _bcdiag->Fill();
      ++_cluIdx;
    }
    ++_iev;
  }

  bool BkgDiag::findData(const art::Event& evt){
    _chcol = 0; _shfcol = 0; _bkgccol = 0; _bkgqcol = 0; _mcdigis = 0;
    // nb: getValidHandle does the protection (exception) on handle validity so I don't have to
    auto chH = evt.getValidHandle(_chToken);
    _chcol = chH.product();
    auto shfH = evt.getValidHandle(_shfToken);
    _shfcol = shfH.product();
    auto bkgcH = evt.getValidHandle(_bkgcToken);
    _bkgccol = bkgcH.product();
    auto bkghH = evt.getValidHandle(_bkghToken);
    _bkghitcol = bkghH.product();
    auto bkgqH = evt.getValidHandle(_bkgqToken);
    _bkgqcol = bkgqH.product();
    if(_mcdiag){
      auto mcdH = evt.getValidHandle(_mcdigisToken);
      _mcdigis = mcdH.product();
    }
    return _chcol != 0 && _shfcol != 0 && _bkgccol != 0 && _bkgqcol != 0
      && (_mcdigis != 0  || !_mcdiag);
  }


  void BkgDiag::findPrimary(std::vector<uint16_t>const& dids, art::Ptr<SimParticle>& pptr,double& pmom, std::vector<int>& icontrib) const {
    // find the unique simparticles which produced these hits
    std::set<art::Ptr<SimParticle> > pp;
    for(auto id : dids) {
      StrawDigiMC const& mcdigi = _mcdigis->at(id);
      art::Ptr<SimParticle> const& spp = mcdigi.earlyStrawGasStep()->simParticle();
      if(spp.isNonnull()){
        pp.insert(spp);
      }
    }
    // map these particles back to each other, to compress out particles generated inside the cluster
    std::map<art::Ptr<SimParticle>,art::Ptr<SimParticle> > spmap;
    // look for particles produced at the same point, like conversions.  It's not enough to look for the same parent,
    // as that parent could produce multiple daughters at different times.  Regardless of mechanism or genealogy, call these 'the same'
    // as they will contribute equally to the spiral
    for(std::set<art::Ptr<SimParticle> >::iterator ipp=pp.begin();ipp!=pp.end();++ipp){
      art::Ptr<SimParticle> sppi = *ipp;
      spmap[sppi] = sppi;
    }
    for(std::set<art::Ptr<SimParticle> >::iterator ipp=pp.begin();ipp!=pp.end();++ipp){
      art::Ptr<SimParticle> sppi = *ipp;
      if(sppi->genParticle().isNull()){
        std::set<art::Ptr<SimParticle> >::iterator jpp=ipp;++jpp;
        for(;jpp!=pp.end();++jpp){
          art::Ptr<SimParticle> sppj = *jpp;
          if(sppj->genParticle().isNull()){
            // call the particles 'the same' if they are related and were produced near each other
            MCRelationship rel(sppi,sppj);
            if(rel==MCRelationship::daughter || rel == MCRelationship::udaughter){
              spmap[sppi] = sppj;
              break;
            } else if(rel == MCRelationship::mother || rel == MCRelationship::umother){
              spmap[sppj] = sppi;
            } else if(rel == MCRelationship::sibling || rel == MCRelationship::usibling){
              double dist = (sppj->startPosition() - sppi->startPosition()).mag();
              if(dist < 10.0){
                if(sppi->id().asInt() > sppj->id().asInt())
                  spmap[sppi] = sppj;
                else
                  spmap[sppj] = sppi;
              }
            }
          }
        }
      }
    }
    // check for remapping
    bool changed(true);
    while(changed){
      changed = false;
      for(std::map<art::Ptr<SimParticle>,art::Ptr<SimParticle> >::iterator im = spmap.begin();im!=spmap.end();++im){
        std::map<art::Ptr<SimParticle>,art::Ptr<SimParticle> >::iterator ifnd = spmap.find(im->second);
        if( !(ifnd->second == ifnd->first)){
          changed = true;
          spmap[im->first] = ifnd->second;
        }
      }
    }
    // find the most likely ultimate parent for this cluster.  Also fill general info
    std::map<int,int> mode;
    for(std::set<art::Ptr<SimParticle> >::iterator ipp=pp.begin();ipp!=pp.end();++ipp){
      art::Ptr<SimParticle> spp = *ipp;
      int mcid(-1);
      // map back to the ultimate parent
      spp = spmap[spp];
      mcid = spp->id().asInt();
      std::map<int,int>::iterator ifnd = mode.find(mcid);
      if(ifnd != mode.end())
        ++(ifnd->second);
      else
        mode[mcid] = 1;
    }
    int max(0);
    std::map<int,int>::iterator imax = mode.end();
    for(std::map<int,int>::iterator im=mode.begin();im!=mode.end();++im){
      icontrib.push_back(im->first);
      if(im->second>max){
        imax=im;
        max = im->second;
      }
    }
    unsigned pid(0);
    if(imax != mode.end())
      pid=imax->first;
    for(std::map<art::Ptr<SimParticle>,art::Ptr<SimParticle> >::iterator im = spmap.begin();im!=spmap.end();++im){
      if(im->first->id().asInt() == pid){
        pptr = im->first;
        break;
      }
    }
    // find the momentum for the first step point from the primary particle in this delta
    for(auto id : dids) {
      StrawDigiMC const& mcdigi = _mcdigis->at(id);
      auto const& sgsp = mcdigi.earlyStrawGasStep();
      art::Ptr<SimParticle> const& spp = sgsp->simParticle();
      if(spp == pptr){
        pmom = sqrt(sgsp->momentum().mag2());
        break;
      }
    }
  }

  void BkgDiag::fillStrawHitInfoMC(StrawDigiMC const& mcdigi, art::Ptr<SimParticle>const& pptr, StrawHitInfo& shinfo) const {
    // use TDC channel 0 to define the MC match
    auto const& sgsp = mcdigi.earlyStrawGasStep();
    art::Ptr<SimParticle> const& spp = sgsp->simParticle();
    shinfo._mct0 = sgsp->time();
    shinfo._mcht = mcdigi.wireEndTime(mcdigi.earlyEnd());
    shinfo._mcpdg = spp->pdgId();
    shinfo._mcproc = spp->creationCode();
    shinfo._mcedep = mcdigi.energySum();
    shinfo._mcgen = -1;
    if(spp->genParticle().isNonnull())
      shinfo._mcgen = spp->genParticle()->generatorId().id();

    shinfo._mcpos = sgsp->position();
    shinfo._mctime = shinfo._mct0;
    shinfo._mcedep = mcdigi.energySum();;
    shinfo._mcmom = sqrt(sgsp->momentum().mag2());
    double cosd = cos(sgsp->momentum().Theta());
    shinfo._mctd = cosd/sqrt(1.0-cosd*cosd);
    // relationship to parent
    shinfo._relation=MCRelationship::none;
    if(sgsp.isNonnull() && pptr.isNonnull()){
      art::Ptr<SimParticle> const& spp = sgsp->simParticle();
      if(spp.isNonnull()){
        MCRelationship rel(spp,pptr);
        shinfo._relation = rel.relationship();
      }
    }
  }

  void BkgDiag::fillStrawHitInfo(size_t ich, StrawHitInfo& shinfo) const {
    ComboHit const& ch = _chcol->at(ich);
    StrawHitFlag shf;
    if(_useflagcol)
      shf = _shfcol->at(ich);
    else
      shf = ch.flag();

    shinfo._stereo = shf.hasAllProperties(StrawHitFlag::stereo);
    shinfo._tdiv = shf.hasAllProperties(StrawHitFlag::tdiv);
    shinfo._esel = shf.hasAllProperties(StrawHitFlag::energysel);
    shinfo._rsel = shf.hasAllProperties(StrawHitFlag::radsel);
    shinfo._tsel = shf.hasAllProperties(StrawHitFlag::timesel);
    shinfo._strawxtalk = shf.hasAllProperties(StrawHitFlag::strawxtalk);
    shinfo._elecxtalk = shf.hasAllProperties(StrawHitFlag::elecxtalk);
    shinfo._isolated = shf.hasAllProperties(StrawHitFlag::isolated);
    shinfo._bkg = shf.hasAllProperties(StrawHitFlag::bkg);

    shinfo._pos = ch.pos();
    shinfo._time = ch.correctedTime();
    shinfo._wdist = ch.wireDist();
    shinfo._wres = ch.posRes(ComboHit::wire);
    shinfo._tres = ch.posRes(ComboHit::trans);
    // info depending on stereo hits
    shinfo._chisq = ch.qual();
    shinfo._edep = ch.energyDep();
    shinfo._dedx = ch.specificIonization();
    StrawId const& sid = ch.strawId();
    shinfo._plane = sid.plane();
    shinfo._panel = sid.panel();
    shinfo._layer = sid.layer();
    shinfo._straw = sid.straw();
    shinfo._stereo = ch.flag().hasAllProperties(StrawHitFlag::stereo);
    shinfo._tdiv = ch.flag().hasAllProperties(StrawHitFlag::tdiv);
  }
} // mu2e namespace

// Part of the magic that makes this class a module.
using mu2e::BkgDiag;
DEFINE_ART_MODULE(BkgDiag);


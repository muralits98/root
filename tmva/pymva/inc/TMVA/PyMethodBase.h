// @(#)root/tmva/rmva $Id$
// Author: Omar Zapata 2015

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : RMethodBase                                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Virtual base class for all MVA method based on ROOTR                      *
 *                                                                                *
 **********************************************************************************/

#ifndef ROOT_TMVA_PyMethodBase
#define ROOT_TMVA_PyMethodBase

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// PyMethodBase                                                         //
//                                                                      //
// Virtual base class for all TMVA method based on scikit-learn         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TMVA_MethodBase
#include "TMVA/MethodBase.h"
#endif

#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

class TGraph;
class TTree;
class TDirectory;
class TSpline;
class TH1F;
class TH1D;

namespace TMVA {

   class Ranking;
   class PDF;
   class TSpline1;
   class MethodCuts;
   class MethodBoost;
   class DataSetInfo;

   class PyMethodBase : public MethodBase {

      friend class Factory;
   public:

      // default constructur
      PyMethodBase( const TString& jobName,
                  Types::EMVA methodType,
                  const TString& methodTitle,
                  DataSetInfo& dsi,
                  const TString& theOption = "",
                  TDirectory* theBaseDir = 0);

      // constructor used for Testing + Application of the MVA, only (no training),
      // using given weight file
      PyMethodBase( Types::EMVA methodType,
                  DataSetInfo& dsi,
                  const TString& weightFile,
                  TDirectory* theBaseDir = 0);

      // default destructur
      virtual ~PyMethodBase();
      virtual void     Train() = 0;
      // options treatment
      virtual void     Init()           = 0;
      virtual void     DeclareOptions() = 0;
      virtual void     ProcessOptions() = 0;
      // create ranking
      virtual const Ranking* CreateRanking() = 0;
      
      virtual Double_t GetMvaValue( Double_t* errLower = 0, Double_t* errUpper = 0) = 0;

     Bool_t HasAnalysisType( Types::EAnalysisType type, UInt_t numberClasses, UInt_t numberTargets )=0;
   protected:
      // the actual "weights"
      virtual void AddWeightsXMLTo      ( void* parent ) const = 0;
      virtual void ReadWeightsFromXML   ( void* wghtnode ) = 0;
      virtual void ReadWeightsFromStream( std::istream& ) = 0;       // backward compatibility
      virtual void ReadWeightsFromStream( TFile& ) {}                // backward compatibility
       
   protected:
     PyObject *fModuleSklearn;
     PyObject *fClassifier;
//      PyArrayObject *fTrainData;
//      PyArrayObject *fTrainDataWeights;//array of weights
//      PyArrayObject *fTrainDataClasses;//array with sig/bgk class
     PyObject *fTrainData;
     PyObject *fTrainDataWeights;//array of weights
     PyObject *fTrainDataClasses;//array with sig/bgk class
   private:
      ClassDef(PyMethodBase,0)  // Virtual base class for all TMVA method

   };
} // namespace TMVA

#endif



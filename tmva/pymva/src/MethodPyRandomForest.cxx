// @(#)root/tmva/pymva $Id$
// Author: Omar Zapata 2015

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : MethodPyRandomForest                                                  *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Random Forest Classifiear from Scikit learn                               *
 *                                                                                *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 *                                                                                *
 **********************************************************************************/

#include <iomanip>

#include "TMath.h"
#include "Riostream.h"
#include "TMatrix.h"
#include "TMatrixD.h"
#include "TVectorD.h"

#include "TMVA/VariableTransformBase.h"
#include "TMVA/MethodPyRandomForest.h"
#include "TMVA/Tools.h"
#include "TMVA/Ranking.h"
#include "TMVA/Types.h"
#include "TMVA/PDF.h"
#include "TMVA/ClassifierFactory.h"

#include "TMVA/Results.h"

using namespace TMVA;

REGISTER_METHOD(PyRandomForest)

ClassImp(MethodPyRandomForest)

static PyObject *kwargs=NULL;
static bool TrainAgain=kFALSE;

//_______________________________________________________________________
MethodPyRandomForest::MethodPyRandomForest(const TString &jobName,
                     const TString &methodTitle,
                     DataSetInfo &dsi,
                     const TString &theOption,
                     TDirectory *theTargetDir) :
   PyMethodBase(jobName, Types::kPyRandomForest, methodTitle, dsi, theOption, theTargetDir),
   n_estimators(10),
   criterion("gini"),
   max_depth("None"),
   min_samples_leaf(1),
   min_weight_fraction_leaf(0),
   bootstrap(kTRUE),
    n_jobs(1)   
{
   // standard constructor for the PyRandomForest


}

//_______________________________________________________________________
MethodPyRandomForest::MethodPyRandomForest(DataSetInfo &theData, const TString &theWeightFile, TDirectory *theTargetDir)
   : PyMethodBase(Types::kPyRandomForest, theData, theWeightFile, theTargetDir),
   n_estimators(10),
   criterion("gini"),
   max_depth("None"),
   min_samples_leaf(1),
   min_weight_fraction_leaf(0),
   bootstrap(kTRUE),
   n_jobs(1)   
{
}


//_______________________________________________________________________
MethodPyRandomForest::~MethodPyRandomForest(void)
{
}

//_______________________________________________________________________
Bool_t MethodPyRandomForest::HasAnalysisType(Types::EAnalysisType type, UInt_t numberClasses, UInt_t numberTargets)
{
    if (type == Types::kClassification && numberClasses == 2) return kTRUE;
    return kFALSE;
}


//_______________________________________________________________________
void MethodPyRandomForest::DeclareOptions()
{
    MethodBase::DeclareCompatibilityOptions();

    DeclareOptionRef(n_estimators, "NEstimators", "Integer, optional (default=10). The number of trees in the forest.");
    DeclareOptionRef(criterion, "Criterion", "//string, optional (default='gini') \
    The function to measure the quality of a split. Supported criteria are \
    'gini' for the Gini impurity and 'entropy' for the information gain. \
    Note: this parameter is tree-specific.");
    DeclareOptionRef(max_depth, "MaxDepth", "nteger, optional (default=2) \
    The minimum number of samples required to split an internal node. \
    Note: this parameter is tree-specific.");
    DeclareOptionRef(min_samples_leaf, "MinSamplesLeaf", "integer, optional (default=1) \
    The minimum number of samples in newly created leaves.  A split is \
    discarded if after the split, one of the leaves would contain less then \
    ``min_samples_leaf`` samples.");
    DeclareOptionRef(min_weight_fraction_leaf, "MinWeightFractionLeaf", "//float, optional (default=0.) \
    The minimum weighted fraction of the input samples required to be at a \
    leaf node.");
    DeclareOptionRef(bootstrap, "Bootstrap", "boolean, optional (default=True) \
    Whether bootstrap samples are used when building trees.");
    DeclareOptionRef(n_jobs, "NJobs", " integer, optional (default=1) \
    The number of jobs to run in parallel for both `fit` and `predict`. \
    If -1, then the number of jobs is set to the number of cores.");
}

//_______________________________________________________________________
void MethodPyRandomForest::ProcessOptions()
{
    if (n_estimators <= 0) {
        Log() << kERROR << " NEstimators <=0... that does not work !! "
        << " I set it to 10 .. just so that the program does not crash"
        << Endl;
        n_estimators = 10;
    }
    //TODO: Error control for variables here  
         kwargs = Py_BuildValue("s=i,s=s,s=s,s=i,s=f,s=i,s=i",\
        "n_estimators",n_estimators,\
        "criterion",criterion, \
        "max_depth",max_depth, \
        "min_samples_leaf",min_samples_leaf, \
        "min_weight_fraction_leaf",min_weight_fraction_leaf,\
        "bootstrap",bootstrap, \
        "n_jobs",n_jobs);//NOTE: not all options was passed
   Log() << kERROR <<"ProcessOptions =" <<n_jobs<<Endl;

}


//_______________________________________________________________________
void  MethodPyRandomForest::Init()
{
        Log() << kERROR <<"INIT =" <<n_jobs<<Endl;
    _import_array();//require to use numpy arrays
    
    //Import sklearn
    // Convert the file name to a Python string.
    PyObject *pName= PyString_FromString("sklearn.ensemble");
    // Import the file as a Python module.
    fModuleSklearn = PyImport_Import(pName);
    Py_DECREF(pName);
    
    if(!fModuleSklearn)
    {
        Log() <<kFATAL<< "Can't import sklearn.ensemble" << Endl;
        Log() << Endl;
    }
    

    //Training data
    UInt_t fNvars=Data()->GetNVariables();
    int fNrowsTraining=Data()->GetNTrainingEvents();//every row is an event, a class type and a weight
    int *dims=new int[2];
    dims[0]=fNrowsTraining;
    dims[1]=fNvars;
    fTrainData=(PyArrayObject*)PyArray_FromDims(2, dims, NPY_FLOAT);
    float* TrainData=(float*)(PyArray_DATA(fTrainData));
    
    
    fTrainDataClasses=(PyArrayObject*)PyArray_FromDims(1,&fNrowsTraining, NPY_FLOAT);
    float* TrainDataClasses=(float*)(PyArray_DATA(fTrainDataClasses));
    
    fTrainDataWeights=(PyArrayObject*)PyArray_FromDims(1,&fNrowsTraining, NPY_FLOAT);
    float* TrainDataWeights=(float*)(PyArray_DATA(fTrainDataWeights));
    
    for(int i=0;i<fNrowsTraining;i++)
    {
        const TMVA::Event *e=Data()->GetTrainingEvent(i);
        for(UInt_t j=0;j<fNvars;j++)
        {
            TrainData[j+i*fNvars]=e->GetValue(j);
        }
        if(e->GetClass()==TMVA::Types::kSignal) TrainDataClasses[i]=TMVA::Types::kSignal;
        else TrainDataClasses[i]=TMVA::Types::kBackground;
        
        TrainDataWeights[i]=e->GetWeight();
    }
}

void MethodPyRandomForest::Train()
{
    ProcessSetup();
    PyObject *pDict = PyModule_GetDict(fModuleSklearn);
    PyObject *fClassifierClass = PyDict_GetItemString(pDict, "RandomForestClassifier");
    
    // Create an instance of the class
    if (PyCallable_Check(fClassifierClass ))
    {
        //instance        
        fClassifier = PyObject_CallObject(fClassifierClass ,kwargs);
        PyObject_Print(fClassifier, stdout, 0);
        
        // //         Py_DECREF(kwargs); 
    }else{
        PyErr_Print();
        Py_DECREF(pDict);
        Py_DECREF(fClassifierClass);
        Log() <<kFATAL<< "Can't call function RandomForestClassifier" << Endl;
        Log() << Endl;
        
    }   
    fClassifier=PyObject_CallMethod(fClassifier,(char*)"fit",(char*)"(OOO)", fTrainData,fTrainDataClasses,fTrainDataWeights);
    //     PyObject_Print(fClassifier, stdout, 0);
    //     pValue =PyObject_CallObject(fClassifier, PyString_FromString("classes_"));
    //     PyObject_Print(pValue, stdout, 0);
    
}

//_______________________________________________________________________
void MethodPyRandomForest::TestClassification()
{
    MethodBase::TestClassification();
}


//_______________________________________________________________________
Double_t MethodPyRandomForest::GetMvaValue(Double_t *errLower, Double_t *errUpper)
{
    // cannot determine error
    NoErrorCalc(errLower, errUpper);
    //NOTE: the testing evaluation is using thread and for unknow reason
    //I need to re-traing because fClassifier if not working on the thread
    //NOTE: appear to be that GetMvaValue is called after load train's macros output
    //with training information from MethodBase::MakeClass TMVAClassification_PyRandomForest.class.C
    //this is not implemented yet
    if(Data()->GetCurrentType() == Types::kTesting)
    {
        if(!TrainAgain)
        {
            _import_array();//require to use numpy arrays(threads need reload)
            Train();
            TrainAgain=kTRUE;
        }
    }
    
    Double_t mvaValue;
    const TMVA::Event *e=Data()->GetEvent();
    UInt_t nvars=e->GetNVariables();
    PyObject *pEvent=PyTuple_New(nvars);
    for(UInt_t i=0;i<nvars;i++){
        
        PyObject *pValue=PyFloat_FromDouble(e->GetValue(i));
        if (!pValue)
        {
            Py_DECREF(pEvent);
            Py_DECREF(fTrainData);
            Log()<<kFATAL<<"Error Evaluating MVA "<<Endl;
        }
        PyTuple_SetItem(pEvent, i,pValue);
    }
    
    PyArrayObject *result=(PyArrayObject*)PyObject_CallMethod(fClassifier,(char*)"predict_proba",(char*)"(O)",pEvent);
    float* proba=(float*)(PyArray_DATA(result));
    mvaValue=proba[1];//getting signal prob
    Py_DECREF(result);
    Py_DECREF(pEvent);
    //    PyObject_Print(result, stdout, 0);
    //    std::cout<<std::endl;
    return mvaValue;
}

//_______________________________________________________________________
void MethodPyRandomForest::GetHelpMessage() const
{
    // get help message text
    //
    // typical length of text line:
    //         "|--------------------------------------------------------------|"
    Log() << Endl;
    Log() << gTools().Color("bold") << "--- Short description:" << gTools().Color("reset") << Endl;
    Log() << Endl;
    Log() << "Decision Trees and Rule-Based Models " << Endl;
    Log() << Endl;
    Log() << gTools().Color("bold") << "--- Performance optimisation:" << gTools().Color("reset") << Endl;
    Log() << Endl;
    Log() << Endl;
    Log() << gTools().Color("bold") << "--- Performance tuning via configuration options:" << gTools().Color("reset") << Endl;
    Log() << Endl;
    Log() << "<None>" << Endl;
}


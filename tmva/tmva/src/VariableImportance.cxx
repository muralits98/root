// @(#)root/tmva $Id$
// Author: Omar Zapata and Sergei Gleyzer

#include <iostream>

#include "TMVA/VariableImportance.h"
#include "TMVA/MethodBase.h"
#include "TSystem.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "TGraph.h"
#include <memory>
#include "TMVA/tmvaglob.h"
#include <bitset>
#include <utility>

//number of bits for bitset
#define NBITS          32

TMVA::VariableImportanceResult::VariableImportanceResult():fImportanceValues("VariableImportance"),fImportanceHist(nullptr)
{
    
}

TMVA::VariableImportanceResult::VariableImportanceResult(const VariableImportanceResult &obj)
{
    fImportanceValues = obj.fImportanceValues;
    fImportanceHist   = obj.fImportanceHist;
}


void TMVA::VariableImportanceResult::Print() const
{
    fImportanceValues.Print();    
}


TCanvas* TMVA::VariableImportanceResult::Draw(const TString name) const
{
    TMVA::TMVAGlob::Initialize();
    TCanvas *c=new TCanvas(name.Data());
    fImportanceHist->Draw("");
    fImportanceHist->GetXaxis()->SetTitle(" Variable Names ");
    fImportanceHist->GetYaxis()->SetTitle(" fImportance (%) ");
    TMVA::TMVAGlob::plot_logo();
    c->Draw();
    return c;
}

TMVA::VariableImportance::VariableImportance(TMVA::DataLoader *dataloader):TMVA::Algorithm(dataloader,"CrossValidation")
{
    fClassifier=std::unique_ptr<Factory>(new TMVA::Factory("VariableImportance","!V:ROC:Silent:Color:!DrawProgressBar:AnalysisType=Classification"));
}

TMVA::VariableImportance::~VariableImportance()
{
    fClassifier=nullptr;
}


void TMVA::VariableImportance::Evaluate()
{
    TString methodName    = fMethod.GetValue<TString>("MethodName");
    TString methodTitle   = fMethod.GetValue<TString>("MethodTitle");
    TString methodOptions = fMethod.GetValue<TString>("MethodOptions");
    EvaluateImportanceShort();
}

ULong_t TMVA::VariableImportance::Sum(ULong_t i)
{
    ULong_t sum=0;
    for(ULong_t n=0;n<i;n++) sum+=pow(2,n);
    return sum;
}

TH1F* TMVA::VariableImportance::GetImportance(const UInt_t nbits,std::vector<Double_t> &importances,std::vector<TString> &varNames)
{
    TH1F *vihist  = new TH1F("vihist", "", nbits, 0, nbits);
    
    gStyle->SetOptStat(000000);
    
    Float_t normalization = 0.0;
    for (UInt_t i = 0; i < nbits; i++) normalization += importances[i];
    
    Float_t roc = 0.0;
    
    gStyle->SetTitleXOffset(0.4);
    gStyle->SetTitleXOffset(1.2);
    
    
    for (UInt_t i = 1; i < nbits + 1; i++) {
        roc = 100.0 * importances[i - 1] / normalization;
        vihist->GetXaxis()->SetBinLabel(i, varNames[i - 1].Data());
        vihist->SetBinContent(i, roc);
    }
    
    vihist->LabelsOption("v >", "X");
    vihist->SetBarWidth(0.97);
    vihist->SetFillColor(TColor::GetColor("#006600"));
    
    vihist->GetYaxis()->SetTitle("Importance (%)");
    vihist->GetYaxis()->SetTitleSize(0.045);
    vihist->GetYaxis()->CenterTitle();
    vihist->GetYaxis()->SetTitleOffset(1.24);
    
    vihist->GetYaxis()->SetRangeUser(-7, 50);
    vihist->SetDirectory(0);
    
    return vihist;
}

void TMVA::VariableImportance::EvaluateImportanceShort()
{    
    TString methodName    = fMethod.GetValue<TString>("MethodName");
    TString methodTitle   = fMethod.GetValue<TString>("MethodTitle");
    TString methodOptions = fMethod.GetValue<TString>("MethodOptions");
    
    uint32_t x = 0;
    uint32_t y = 0;
    //getting number of variables and variable names from loader
    const UInt_t nbits = fDataLoader->GetDefaultDataSetInfo().GetNVariables();
    std::vector<TString> varNames = fDataLoader->GetDefaultDataSetInfo().GetListOfVariables();
    
    ULong_t range = Sum(nbits);
    
    //vector to save importances
    std::vector<Double_t> importances(nbits);
    for (UInt_t i = 0; i < nbits; i++)importances[i] = 0;
    
    Float_t SROC, SSROC; //computed ROC value for every Seed and SubSeed
    
    x = range;
    
    std::bitset<NBITS>  xbitset(x);
    if (x == 0) Log()<<kFATAL<<"Error: need at least one variable."; //dataloader need at least one variable
    
    
    //creating loader for seed
    TMVA::DataLoader *seeddl = new TMVA::DataLoader(xbitset.to_string());
    
    //adding variables from seed
    for (UInt_t index = 0; index < nbits; index++){ 
        if (xbitset[index]) seeddl->AddVariable(varNames[index], 'F');
    }
    
    //Loading Dataset
    DataLoaderCopy(seeddl,fDataLoader.get());
    
    //Booking Seed
    fClassifier->BookMethod(seeddl, methodName, methodTitle, methodOptions);
    
    //Train/Test/Evaluation
    fClassifier->TrainAllMethods();
    fClassifier->TestAllMethods();
    fClassifier->EvaluateAllMethods();
    
    //getting ROC
    SROC = fClassifier->GetROCIntegral(xbitset.to_string(), methodTitle);
    
    delete seeddl;  
    fClassifier->DeleteAllMethods();
        
    for (uint32_t i = 0; i < NBITS; ++i) {
        if (x & (1 << i)) {
            y = x & ~(1 << i);
            std::bitset<NBITS>  ybitset(y);
            //need at least one variable
            //NOTE: if subssed is zero then is the special case
            //that count in xbitset is 1
            Double_t ny = log(x - y) / 0.693147;
            if (y == 0) {
                importances[ny] = SROC - 0.5;
                continue;
            }
            
            //creating loader for subseed
            TMVA::DataLoader *subseeddl = new TMVA::DataLoader(ybitset.to_string());
            //adding variables from subseed
            for (UInt_t index = 0; index < nbits; index++) {
                if (ybitset[index]) subseeddl->AddVariable(varNames[index], 'F');
            }
            
            //Loading Dataset
            DataLoaderCopy(subseeddl,fDataLoader.get());
            
            //Booking SubSeed
            fClassifier->BookMethod(subseeddl, methodName, methodTitle, methodOptions);
            
            //Train/Test/Evaluation
            fClassifier->TrainAllMethods();
            fClassifier->TestAllMethods();
            fClassifier->EvaluateAllMethods();
            
            //getting ROC
            SSROC = fClassifier->GetROCIntegral(ybitset.to_string(), methodTitle);
            importances[ny] += SROC - SSROC;
            
            delete subseeddl;
            fClassifier->DeleteAllMethods();
        }
    }
    Float_t normalization = 0.0;
    for (UInt_t i = 0; i < nbits; i++) normalization += importances[i];
    
    for(UInt_t i=0;i<nbits;i++){
        //adding values
        fResults.fImportanceValues[varNames[i]]=(100.0 * importances[i] / normalization);
        //adding sufix
        fResults.fImportanceValues[varNames[i]]=fResults.fImportanceValues.GetValue<TString>(varNames[i])+" % ";
    }
    fResults.fImportanceHist = std::shared_ptr<TH1F>(GetImportance(nbits,importances,varNames));
}



//
// Created by Mike on 8/30/14.
//
#pragma once

#include <QObject>
#include "Root.h"

class QtVGMRoot
        : public QObject, public VGMRoot
{
    Q_OBJECT

public:
    QtVGMRoot();
    ~QtVGMRoot() override;

    void UI_SetRootPtr(VGMRoot** theRoot) override;
    void UI_PreExit() override;
    void UI_Exit() override;
    void UI_AddRawFile(RawFile* newFile) override;
    void UI_CloseRawFile(RawFile* targFile) override;

    void UI_OnBeginScan() override;
    void UI_SetScanInfo() override;
    void UI_OnEndScan() override;
    void UI_AddVGMFile(VGMFile* theFile) override;
    void UI_AddVGMSeq(VGMSeq* theSeq) override;
    void UI_AddVGMInstrSet(VGMInstrSet* theInstrSet) override;
    void UI_AddVGMSampColl(VGMSampColl* theSampColl) override;
    void UI_AddVGMMisc(VGMMiscFile* theMiscFile) override;
    void UI_AddVGMColl(VGMColl* theColl) override;
    void UI_AddLogItem(LogItem* theLog) override;
    void UI_RemoveVGMFile(VGMFile* targFile) override;
    void UI_RemoveVGMColl(VGMColl* targColl) override;
    void UI_BeginRemoveVGMFiles() override;
    void UI_EndRemoveVGMFiles() override;
    //virtual void UI_RemoveVGMFileRange(VGMFile* first, VGMFile* last);
    void UI_AddItem(VGMItem* item, VGMItem* parent, const std::wstring& itemName, void* UI_specific) override;
    void UI_AddItemSet(VGMFile* file, std::vector<ItemSet>* itemset) override;
    std::wstring UI_GetOpenFilePath(const std::wstring& suggestedFilename = L"", const std::wstring& extension = L"") override;
    std::wstring UI_GetSaveFilePath(const std::wstring& suggestedFilename, const std::wstring& extension = L"") override;
    std::wstring UI_GetSaveDirPath(const std::wstring& suggestedDir = L"") override;

signals:
    void UI_AddedRawFile();
    void UI_RemovedRawFile();
    void UI_AddedVGMFile();
    void UI_RemovedVGMFile();
    void UI_AddedVGMColl();
    void UI_RemovedVGMColl();
};

extern QtVGMRoot qtVGMRoot;
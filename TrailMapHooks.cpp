// TrailMap - Hook into oCDocumentManager::Show for faster map detection
// Union SOURCE file

namespace GOTHIC_ENGINE {

    HOOK Ivk_TM_oCDocumentManager_Show PATCH(&oCDocumentManager::Show, &oCDocumentManager::Show_TrailMap);
    void __fastcall oCDocumentManager::Show_TrailMap(int id)
    {
        THISCALL(Ivk_TM_oCDocumentManager_Show)(id);

        if (trailMap && trailMap->enabled && trailMap->saveSlot >= 0) {
            if (auto docView = this->GetDocumentView(id)) {
                trailMap->TryInitFromDoc(docView);
            }
        }
    }
}

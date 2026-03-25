// TrailMap - Heatmap overlay for Gothic 2 map
// Union SOURCE file

namespace GOTHIC_ENGINE {

    std::unique_ptr<TrailMap> trailMap;

    // helper: clamp value between lo and hi
    static int tmClamp(int val, int lo, int hi) {
        if (val < lo) return lo;
        if (val > hi) return hi;
        return val;
    }

    // ================================================================
    //  CTOR / DTOR
    // ================================================================
    TrailMap::TrailMap() {
        viewDot   = new zCView(0, 0, 0, 0);
        viewPanel = new zCView(0, 0, 0, 0);
        viewPanel->InsertBack("ITEMMAP_BACKGROUND.TGA");
        viewPanel->SetAlphaBlendFunc(zTRnd_AlphaBlendFunc::zRND_ALPHA_FUNC_BLEND);

        mapActive       = false;
        showHeatmap     = true;
        showPanel       = true;
        transparentPanel= false;
        hook            = TM_NoHook;
        mapRotate       = 0;
        mapCoords       = zVEC4(0,0,0,0);
        worldCoords     = zVEC4(0,0,0,0);
        markX = markY   = 0;
        saveSlot        = -1;
        for (int i = 0; i < 10; i++) stepCounts[i] = 0;
        filter          = TF_ALL;
        enabled         = true;
        gridSize        = 500;
        maxVisitsForColor=5;
        dotSize         = 8;

        zSTRING texMark = "L.TGA";
        zCTexture* tmp = tmp->Load(texMark, True);
        if (tmp) {
            tmp->GetPixelSize(markX, markY);
            tmp->Release();
            tmp = 0;
        }

        lastInitTime   = std::chrono::steady_clock::now();
        lastPlayerPos  = zVEC3(0, 0, 0);
        lastCellKey.gx = INT_MIN;  // Initialize to invalid value
        lastCellKey.gz = INT_MIN;

        // CoM Ikarus map symbol indices
        indexSpriteMapHandle    = parser->GetIndex("SPRITEMAP_SPRITEHNDL");
        indexSpriteCursorHandle = parser->GetIndex("SPRITEMAP_SPRITECURSORHNDL");
    }

    TrailMap::~TrailMap() {
        if (viewDot)   { delete viewDot;   viewDot   = Null; }
        if (viewPanel) { delete viewPanel; viewPanel = Null; }
    }

    // ================================================================
    //  INIT / OPTIONS
    // ================================================================
    void TrailMap::Init() {
        ReadOptions();
    }

    void TrailMap::ReadOptions() {
        if (!zoptions) return;
        enabled          = zoptions->ReadBool  ("TRAILMAP", "Enabled",          True);
        showHeatmap      = zoptions->ReadBool  ("TRAILMAP", "ShowHeatmap",       True);
        showPanel        = zoptions->ReadBool  ("TRAILMAP", "ShowPanel",         True);
        transparentPanel = zoptions->ReadBool  ("TRAILMAP", "TransparentPanel",  False);
        filter           = (TrailFilter)tmClamp(zoptions->ReadInt("TRAILMAP", "PrevFilter", TF_ALL), 0, TF_MAX);

        gridSize = zoptions->ReadInt("TRAILMAP", "GridSize", 500);
        if (gridSize < 100) gridSize = 100;

        // Choice box items still store index, map to actual values
        // MaxVisitsForColor choices: "1|3|5|10|20|50|100"  (indices 0-6, default=2 -> 5)
        static const int maxVisValues[]  = { 1, 3, 5, 10, 20, 50, 100 };
        int maxVisIdx = tmClamp(zoptions->ReadInt("TRAILMAP", "MaxVisitsForColor", 2), 0, 6);
        maxVisitsForColor = maxVisValues[maxVisIdx];

        // DotSize choices: "3|5|8|12|16|20|30"  (indices 0-6, default=2 -> 8)
        static const int dotValues[]     = { 3, 5, 8, 12, 16, 20, 30 };
        int dotIdx = tmClamp(zoptions->ReadInt("TRAILMAP", "DotSize", 2), 0, 6);
        dotSize = dotValues[dotIdx];
    }

    // ================================================================
    //  CHAPTER DETECTION
    // ================================================================
    int TrailMap::GetCurrentChapter() {
        if (!parser) return 1;
        zCPar_Symbol* sym = parser->GetSymbol("KAPITEL");
        if (!sym) {
            sym = parser->GetSymbol("CURRENTCHAPTER");
        }
        int ch = sym ? sym->single_intdata : 1;
        return tmClamp(ch, 1, 9);
    }

    // ================================================================
    //  CoM DETECTION & EFFECTIVE GRID SIZE
    // ================================================================
    bool TrailMap::IsCoMActive() {
        if (indexSpriteMapHandle == Invalid || indexSpriteCursorHandle == Invalid)
            return false;
        // CoM symbols exist in this mod
        return true;
    }

    int TrailMap::GetEffectiveGridSize() {
        return IsCoMActive() ? gridSize * 2 : gridSize;
    }

    // ================================================================
    //  WORLD KEY
    // ================================================================
    std::string TrailMap::GetWorldKey() {
        if (!ogame || !ogame->GetGameWorld()) return "UNKNOWN";
        zSTRING wn = ogame->GetGameWorld()->GetWorldFilename();
        wn.Upper();
        return std::string(wn.ToChar());
    }

    // ================================================================
    //  FILE PATH
    // ================================================================
    //  RECORD POSITION
    // ================================================================
    void TrailMap::RecordPosition() {
        if (!enabled) return;
        if (!ogame)   return;
        if (ogame->inScriptStartup || ogame->inLoadSaveGame || ogame->inLevelChange) return;
        if (!player)  return;

        zVEC3 pos = player->GetPositionWorld();
        
        // Calculate distance moved since last position
        float dx = pos[VX] - lastPlayerPos[VX];
        float dz = pos[VZ] - lastPlayerPos[VZ];
        float distMoved = std::sqrt(dx * dx + dz * dz);
        
        // Record position and count as a step if moved more than 100 units
        if (distMoved > 100.0f) {
            int ch = GetCurrentChapter();
            stepCounts[ch]++;
            lastPlayerPos = pos;

            // Calculate current grid cell
            int gs = GetEffectiveGridSize();
            int gx = (int)std::floor(pos[VX] / gs);
            int gz = (int)std::floor(pos[VZ] / gs);

            // Only record if we moved to a NEW cell
            if (gx != lastCellKey.gx || gz != lastCellKey.gz) {
                std::string world = GetWorldKey();
                if (world != "UNKNOWN") {
                    CellKey key;
                    key.gx = gx;
                    key.gz = gz;
                    
                    // Record visit to this cell
                    data[world][key].chapterVisits[ch]++;
                    
                    // Update last visited cell
                    lastCellKey = key;
                }
            }
        }
    }

    // ================================================================
    //  SAVE / LOAD using zCArchiver (Gothic save system)
    // ================================================================
    void TrailMap::SaveData(int slotId) {
        if (!enabled) return;
        
        // Get save directory path using SaveLoadGameInfo
        zSTRING savesDir = zoptions->GetDirString(zTOptionPaths::DIR_SAVEGAMES);
        zSTRING slotDir = SaveLoadGameInfo.GetSaveSlotName(slotId);
        zSTRING savePath = savesDir + "\\" + slotDir + "\\TRAILMAP.SAV";
        
        // Create archiver for writing
        zCArchiver* ar = zarcFactory->CreateArchiverWrite(savePath, zARC_MODE_ASCII, 0, 0);
        if (!ar) return;
        
        // Write version and per-chapter step counts
        ar->WriteInt("version", 2);
        for (int i = 1; i <= 9; i++) {
            char scKey[32];
            sprintf(scKey, "stepCount_ch%d", i);
            ar->WriteInt(scKey, stepCounts[i]);
        }
        ar->WriteInt("gridSize", gridSize);
        
        // Write world count
        ar->WriteInt("worldCount", data.size());
        
        int worldIdx = 0;
        for (std::map<std::string, std::map<CellKey, CellData> >::iterator wit = data.begin(); wit != data.end(); ++wit) {
            char worldKey[64];
            sprintf(worldKey, "world%d_name", worldIdx);
            ar->WriteString(worldKey, zSTRING(wit->first.c_str()));
            
            sprintf(worldKey, "world%d_cellCount", worldIdx);
            ar->WriteInt(worldKey, wit->second.size());
            
            int cellIdx = 0;
            for (std::map<CellKey, CellData>::iterator cit = wit->second.begin(); cit != wit->second.end(); ++cit) {
                char cellKey[64];
                sprintf(cellKey, "world%d_cell%d_gx", worldIdx, cellIdx);
                ar->WriteInt(cellKey, cit->first.gx);
                
                sprintf(cellKey, "world%d_cell%d_gz", worldIdx, cellIdx);
                ar->WriteInt(cellKey, cit->first.gz);
                
                sprintf(cellKey, "world%d_cell%d_chapterCount", worldIdx, cellIdx);
                ar->WriteInt(cellKey, cit->second.chapterVisits.size());
                
                int chIdx = 0;
                for (std::map<int,int>::iterator chit = cit->second.chapterVisits.begin(); chit != cit->second.chapterVisits.end(); ++chit) {
                    char chKey[64];
                    sprintf(chKey, "world%d_cell%d_ch%d_num", worldIdx, cellIdx, chIdx);
                    ar->WriteInt(chKey, chit->first);
                    
                    sprintf(chKey, "world%d_cell%d_ch%d_visits", worldIdx, cellIdx, chIdx);
                    ar->WriteInt(chKey, chit->second);
                    chIdx++;
                }
                cellIdx++;
            }
            worldIdx++;
        }
        
        ar->Close();
        ar->Release();
    }

    void TrailMap::LoadData(int slotId) {
        ClearAllData();
        if (!enabled) return;
        
        // Get save directory path using SaveLoadGameInfo
        zSTRING savesDir = zoptions->GetDirString(zTOptionPaths::DIR_SAVEGAMES);
        zSTRING slotDir = SaveLoadGameInfo.GetSaveSlotName(slotId);
        zSTRING savePath = savesDir + "\\" + slotDir + "\\TRAILMAP.SAV";
        
        // Create archiver for reading
        zCArchiver* ar = zarcFactory->CreateArchiverRead(savePath, 0);
        if (!ar) return;
        
        // Read version and per-chapter step counts
        int version = ar->ReadInt("version");
        if (version == 2) {
            for (int i = 1; i <= 9; i++) {
                char scKey[32];
                sprintf(scKey, "stepCount_ch%d", i);
                stepCounts[i] = ar->ReadInt(scKey);
            }
        } else if (version == 1) {
            // Legacy: single stepCount -> assign to chapter 1
            int legacy = ar->ReadInt("stepCount");
            stepCounts[1] = legacy;
        } else {
            ar->Close();
            ar->Release();
            return;
        }
        int savedGridSize = ar->ReadInt("gridSize");
        
        int worldCount = ar->ReadInt("worldCount");
        
        for (int worldIdx = 0; worldIdx < worldCount; worldIdx++) {
            char worldKey[64];
            sprintf(worldKey, "world%d_name", worldIdx);
            zSTRING worldName = ar->ReadString(worldKey);
            std::string worldStr(worldName.ToChar());
            
            sprintf(worldKey, "world%d_cellCount", worldIdx);
            int cellCount = ar->ReadInt(worldKey);
            
            for (int cellIdx = 0; cellIdx < cellCount; cellIdx++) {
                char cellKey[64];
                sprintf(cellKey, "world%d_cell%d_gx", worldIdx, cellIdx);
                int gx = ar->ReadInt(cellKey);
                
                sprintf(cellKey, "world%d_cell%d_gz", worldIdx, cellIdx);
                int gz = ar->ReadInt(cellKey);
                
                CellKey ck;
                ck.gx = gx;
                ck.gz = gz;
                
                sprintf(cellKey, "world%d_cell%d_chapterCount", worldIdx, cellIdx);
                int chapterCount = ar->ReadInt(cellKey);
                
                CellData cd;
                for (int chIdx = 0; chIdx < chapterCount; chIdx++) {
                    char chKey[64];
                    sprintf(chKey, "world%d_cell%d_ch%d_num", worldIdx, cellIdx, chIdx);
                    int chNum = ar->ReadInt(chKey);
                    
                    sprintf(chKey, "world%d_cell%d_ch%d_visits", worldIdx, cellIdx, chIdx);
                    int visits = ar->ReadInt(chKey);
                    
                    if (chNum >= 1 && chNum <= 9 && visits > 0) {
                        cd.chapterVisits[chNum] = visits;
                    }
                }
                
                if (!cd.chapterVisits.empty()) {
                    data[worldStr][ck] = cd;
                }
            }
        }
        
        ar->Close();
        ar->Release();
    }

    void TrailMap::ClearAllData() {
        data.clear();
        for (int i = 0; i < 10; i++) stepCounts[i] = 0;
    }

    // ================================================================
    //  FILTERED COUNT HELPER
    // ================================================================
    int TrailMap::GetFilteredCount(const CellData& cell) {
        switch (filter) {
        case TF_NONE:      return 0;
        case TF_ALL:       return cell.TotalCount();
        case TF_CURRENT:   return cell.CountFor(GetCurrentChapter());
        case TF_CHAPTER_1: return cell.CountFor(1);
        case TF_CHAPTER_2: return cell.CountFor(2);
        case TF_CHAPTER_3: return cell.CountFor(3);
        case TF_CHAPTER_4: return cell.CountFor(4);
        case TF_CHAPTER_5: return cell.CountFor(5);
        case TF_CHAPTER_6: return cell.CountFor(6);
        case TF_CHAPTER_7: return cell.CountFor(7);
        case TF_CHAPTER_8: return cell.CountFor(8);
        case TF_CHAPTER_9: return cell.CountFor(9);
        default: return 0;
        }
    }

    // ================================================================
    //  HEATMAP COLOR
    // ================================================================
    zCOLOR TrailMap::GetHeatColor(int visits) {
        if (visits <= 0) return zCOLOR(0, 0, 0, 0);

        float t = (float)visits / (float)maxVisitsForColor;
        if (t > 1.0f) t = 1.0f;

        // Light green (#80FF80) -> Dark green (#004400)
        int r = (int)(128.0f * (1.0f - t));
        int g = (int)(255.0f * (1.0f - t) + 68.0f * t);
        int b = (int)(128.0f * (1.0f - t));

        if (r < 0) r = 0;
        if (g < 50) g = 50;
        if (g > 255) g = 255;
        if (b < 0) b = 0;

        return zCOLOR(r, g, b);
    }

    // ================================================================
    //  COORDINATE CONVERSION (same logic as ItemMap)
    // ================================================================
    zPOS TrailMap::WorldToScreen(float wx, float wz) {
        zVEC2 mapCenter((mapCoords[0] + mapCoords[2]) / 2.0f,
                        (mapCoords[1] + mapCoords[3]) / 2.0f);
        zVEC2 worldDim(worldCoords[2] - worldCoords[0],
                       worldCoords[3] - worldCoords[1]);

        zVEC2 world2map((mapCoords[2] - mapCoords[0]) / worldDim[0],
                        (mapCoords[3] - mapCoords[1]) / worldDim[1]);

        int x = 0, y = 0;

#if ENGINE <= Engine_G1A
        // Gothic 1: center-based mapping with flipped Y
        world2map[1] = world2map[1] * -1.0f;
        x = (int)((world2map[0] * wx) + mapCenter[0]);
        y = (int)((world2map[1] * wz) + mapCenter[1]);
#else
        // Gothic 2: edge-based mapping
        x = (int)(mapCoords[0] + (world2map[0] * (wx - worldCoords[0])));
        y = (int)(mapCoords[3] - (world2map[1] * (wz - worldCoords[1])));
#endif

        if (mapRotate) {
            int nx = (int)(0.0f*(x-mapCenter[0]) - 1.0f*(y-mapCenter[1]) + mapCenter[0]);
            int ny = (int)(1.0f*(x-mapCenter[0]) + 0.0f*(y-mapCenter[1]) + mapCenter[1]);
            x = nx; y = ny;
        }

        zPOS pos;
        pos.X = screen->anx(x);
        pos.Y = screen->any(y);

        if (hook != TM_CoM && hook != TM_NoMap) {
            pos.X += screen->anx(markX / 2);
            pos.Y += screen->any(markY / 2);
        }
        return pos;
    }

    // ================================================================
    //  MAP DOCUMENT DETECTION
    // ================================================================
    bool TrailMap::TryInitFromDoc(oCViewDocument* doc) {
        oCViewDocumentMap* docMap = dynamic_cast<oCViewDocumentMap*>(doc);
        if (!docMap || !docMap->ViewPageMap) return false;

        zSTRING mapLvl = docMap->Level;
        mapLvl.Replace("/", "\\");
        zSTRING worldLvl = ogame->GetGameWorld()->GetWorldFilename();
        worldLvl.Replace("/", "\\");
        if (!worldLvl.CompareI(mapLvl)) return false;

        auto mapView = docMap->ViewPageMap;
        auto mapPos  = mapView->PixelPosition;
        auto mapSz   = mapView->PixelSize;

        mapCoords = zVEC4(
            (float)mapPos.X,
            (float)mapPos.Y,
            (float)(mapPos.X + mapSz.X),
            (float)(mapPos.Y + mapSz.Y));

        worldCoords = zVEC4(0,0,0,0);
#if ENGINE >= Engine_G2
        worldCoords[0] = docMap->LevelCoords[0];
        worldCoords[1] = docMap->LevelCoords[3];
        worldCoords[2] = docMap->LevelCoords[2];
        worldCoords[3] = docMap->LevelCoords[1];
#endif
        if (worldCoords.Length() == 0.0f) {
            zTBBox3D& wb = ogame->GetGameWorld()->bspTree.bspRoot->bbox3D;
            worldCoords = zVEC4(wb.mins[0], wb.mins[2], wb.maxs[0], wb.maxs[2]);
        }

        ActivateOverlay(TM_Normal);
        return true;
    }

    // ================================================================
    //  CoM (Chronicles of Myrtana) IKARUS MAP HACK
    // ================================================================
    void TrailMap::CoMHack() {
        if (mapActive || hook != TM_NoHook) {
            return;
        }

        if (indexSpriteMapHandle == Invalid || indexSpriteCursorHandle == Invalid) {
            return;
        }

        int SpriteMapHandle    = parser->GetSymbol(indexSpriteMapHandle)->single_intdata;
        int SpriteCursorHandle = parser->GetSymbol(indexSpriteCursorHandle)->single_intdata;

        if (!SpriteMapHandle || !SpriteCursorHandle) {
            return;
        }

        zCPar_Symbol* sym = Null;

        sym = parser->GetSymbol("SPRITEMAP_ROTATE90");
        int SpriteRotate = (sym) ? sym->single_intdata : Invalid;

        sym = parser->GetSymbol("SPRITEMAP_POSX");
        int SpriteMapPosX = (sym) ? sym->single_intdata : Invalid;

        sym = parser->GetSymbol("SPRITEMAP_POSY");
        int SpriteMapPosY = (sym) ? sym->single_intdata : Invalid;

        sym = parser->GetSymbol("SPRITEMAP_SIZE");
        int SpriteMapSize = (sym) ? (sym->single_intdata / 2) : Invalid;

        if (SpriteRotate == Invalid || SpriteMapPosX == Invalid || SpriteMapPosY == Invalid || SpriteMapSize == Invalid) {
            return;
        }

        float SpriteMapTopX    = (float)(SpriteMapPosX - SpriteMapSize);
        float SpriteMapTopY    = (float)(SpriteMapPosY - SpriteMapSize);
        float SpriteMapBottomX = (float)(SpriteMapPosX + SpriteMapSize);
        float SpriteMapBottomY = (float)(SpriteMapPosY + SpriteMapSize);

        mapCoords = zVEC4(SpriteMapTopX, SpriteMapTopY, SpriteMapBottomX, SpriteMapBottomY);

        sym = parser->GetSymbol("SPRITEMAP_MINXF");
        worldCoords[0] = (sym) ? sym->single_floatdata : 0.0f;

        sym = parser->GetSymbol("SPRITEMAP_MINYF");
        worldCoords[1] = (sym) ? sym->single_floatdata : 0.0f;

        sym = parser->GetSymbol("SPRITEMAP_DISTXF");
        worldCoords[2] = (sym) ? (worldCoords[0] + sym->single_floatdata) : 0.0f;

        sym = parser->GetSymbol("SPRITEMAP_DISTYF");
        worldCoords[3] = (sym) ? (worldCoords[1] + sym->single_floatdata) : 0.0f;

        player->CloseInventory();
        player->CloseDeadNpc();
        player->CloseSteal();
        player->CloseTradeContainer();

        ActivateOverlay(TM_CoM, SpriteRotate);
    }

    bool TrailMap::DetectMapDocument() {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastInitTime).count() < 300)
            return false;

        oCDocumentManager& docMan = oCDocumentManager::GetDocumentManager();
        auto docList = docMan.ListDocuments->next;
        bool foundMap = false;

        while (docList) {
            auto doc = docList->GetData();
            docList  = docList->next;
            if (!doc || !doc->HasOpened) continue;
            if (dynamic_cast<oCViewDocumentMap*>(doc)) {
                foundMap = true;
                if (!mapActive) TryInitFromDoc(doc);
                break;
            }
        }
        return foundMap;
    }

    void TrailMap::ActivateOverlay(TM_HookType hookType, int rotate) {
        mapActive = true;
        hook      = hookType;
        mapRotate = rotate;
    }

    void TrailMap::DeactivateOverlay(bool clearKeys) {
        lastInitTime = std::chrono::steady_clock::now();
        mapActive    = false;
        hook         = TM_NoHook;
        mapCoords    = zVEC4(0,0,0,0);
        worldCoords  = zVEC4(0,0,0,0);

        zoptions->WriteBool("TRAILMAP", "ShowHeatmap",      showHeatmap, 0);
        zoptions->WriteBool("TRAILMAP", "ShowPanel",        showPanel,   0);
        zoptions->WriteBool("TRAILMAP", "TransparentPanel", transparentPanel, 0);
        zoptions->WriteInt ("TRAILMAP", "PrevFilter", (int)filter, 0);

        if (clearKeys) zinput->ClearKeyBuffer();
    }

    // ================================================================
    //  INPUT HANDLING
    // ================================================================
    void TrailMap::HandleInput() {
        if (!mapActive) return;

        // F9 - toggle heatmap
        if (zKeyToggled(KEY_F9)) {
            showHeatmap = !showHeatmap;
        }

        // F10 - toggle filter panel
        if (zKeyToggled(KEY_F10)) {
            showPanel = !showPanel;
        }

        // Home / End - cycle chapter filter (skip chapters higher than current)
        int currentChapter = GetCurrentChapter();
        
        if (zKeyToggled(KEY_HOME)) {
            int f = (int)filter;
            do {
                f--;
                if (f < 0) f = TF_MAX;
            } while (f >= TF_CHAPTER_1 && f <= TF_CHAPTER_9 && (f - TF_CHAPTER_1 + 1) > currentChapter);
            filter = (TrailFilter)f;
        }
        
        if (zKeyToggled(KEY_END)) {
            int f = (int)filter;
            do {
                f++;
                if (f > TF_MAX) f = 0;
            } while (f >= TF_CHAPTER_1 && f <= TF_CHAPTER_9 && (f - TF_CHAPTER_1 + 1) > currentChapter);
            filter = (TrailFilter)f;
        }
    }

    // ================================================================
    //  RENDER HEATMAP MARKERS
    // ================================================================
    void TrailMap::RenderHeatmap() {
        if (!showHeatmap || filter == TF_NONE) return;

        std::string world = GetWorldKey();
        std::map<std::string, std::map<CellKey, CellData> >::iterator wit = data.find(world);
        if (wit == data.end()) return;

        int halfDot = screen->anx(dotSize) / 2;

        viewDot->ClrPrintwin();
        viewDot->SetSize(screen->anx(dotSize), screen->any(dotSize));
        viewDot->SetAlphaBlendFunc(zTRnd_AlphaBlendFunc::zRND_ALPHA_FUNC_BLEND);
        viewDot->SetTransparency(200);
        screen->InsertItem(viewDot);

        for (std::map<CellKey, CellData>::iterator cit = wit->second.begin(); cit != wit->second.end(); ++cit) {
            int cnt = GetFilteredCount(cit->second);
            if (cnt <= 0) continue;

            // Cell center in world coordinates
            int gs = GetEffectiveGridSize();
            float wx = (cit->first.gx + 0.5f) * gs;
            float wz = (cit->first.gz + 0.5f) * gs;

            zPOS sp = WorldToScreen(wx, wz);

            // Cull off-screen
            if (sp.X < 0 || sp.X > 8192 || sp.Y < 0 || sp.Y > 8192) continue;

            viewDot->InsertBack("ITEMMAP_MARKER.TGA");
            viewDot->SetPos(sp.X - halfDot, sp.Y - halfDot);
            viewDot->SetColor(GetHeatColor(cnt));
            viewDot->Blit();
        }

        screen->RemoveItem(viewDot);
    }

    // ================================================================
    //  RENDER FILTER PANEL (LEFT SIDE)
    // ================================================================
    void TrailMap::RenderPanel() {
        if (!showPanel) return;

        viewPanel->ClrPrintwin();

        // Panel dimensions - LEFT side of the map
        int panelW = 1800;
        int panelH = 2200;
        int panelX = screen->anx((int)mapCoords[0]) - panelW - 100;
        int panelY = (8192 - panelH) / 2;

        if (panelX < 0)  panelX = 0;
        if (panelX <= 0) panelX = 50;

        screen->InsertItem(viewPanel);
        viewPanel->SetPos(panelX, panelY);
        viewPanel->SetSize(panelW, panelH);
        viewPanel->SetTransparency(transparentPanel ? 128 : 255);

        int margin = 200;
        int y = margin;

        // Title
        viewPanel->SetFont("FONT_OLD_20_WHITE_HI.TGA");
        int bigFontH = viewPanel->FontY();

        viewPanel->SetFontColor(zCOLOR(255, 140, 0));
        zSTRING title = "TrailMap";
        int titleX = (8192 / 2) - (viewPanel->FontSize(title) / 2);
        viewPanel->Print(titleX, y, title);

        // Switch to small font
        viewPanel->SetFont("FONT_OLD_10_WHITE_HI.TGA");
        int fontH = viewPanel->FontY();
        y += bigFontH + fontH;

        // ── Filter with arrows ──
        viewPanel->SetFontColor(zCOLOR(192, 192, 192));

        int fi = (int)filter;
        zSTRING filterName = TrailFilterNames[fi];

        if (fi > 0) {
            zSTRING arrow = "<<";
            viewPanel->Print(margin, y, arrow);
        }
        if (fi < TF_MAX) {
            zSTRING arrow = ">>";
            viewPanel->Print(8192 - viewPanel->FontSize(arrow) - margin, y, arrow);
        }

        int fnW = viewPanel->FontSize(filterName);
        viewPanel->SetFontColor(zCOLOR(255, 255, 255));
        viewPanel->Print((8192 / 2) - (fnW / 2), y, filterName);
        y += fontH + fontH;

        // ── Stats ──
        std::string world = GetWorldKey();
        std::map<std::string, std::map<CellKey, CellData> >::iterator wit = data.find(world);

        int cellCount = 0;

        if (wit != data.end()) {
            for (std::map<CellKey, CellData>::iterator cit = wit->second.begin(); cit != wit->second.end(); ++cit) {
                int c = GetFilteredCount(cit->second);
                if (c > 0) {
                    cellCount++;
                }
            }
        }

        viewPanel->SetFontColor(zCOLOR(144, 238, 144));
        char statBuf[64];

        sprintf(statBuf, "Cells: %d", cellCount);
        viewPanel->Print(margin, y, zSTRING(statBuf));
        y += fontH;

        int displaySteps = 0;
        switch (filter) {
        case TF_ALL:       for (int i = 1; i <= 9; i++) displaySteps += stepCounts[i]; break;
        case TF_CURRENT:   displaySteps = stepCounts[GetCurrentChapter()]; break;
        case TF_CHAPTER_1: displaySteps = stepCounts[1]; break;
        case TF_CHAPTER_2: displaySteps = stepCounts[2]; break;
        case TF_CHAPTER_3: displaySteps = stepCounts[3]; break;
        case TF_CHAPTER_4: displaySteps = stepCounts[4]; break;
        case TF_CHAPTER_5: displaySteps = stepCounts[5]; break;
        case TF_CHAPTER_6: displaySteps = stepCounts[6]; break;
        case TF_CHAPTER_7: displaySteps = stepCounts[7]; break;
        case TF_CHAPTER_8: displaySteps = stepCounts[8]; break;
        case TF_CHAPTER_9: displaySteps = stepCounts[9]; break;
        default: displaySteps = 0; break;
        }
        sprintf(statBuf, "Steps: %d", displaySteps);
        viewPanel->Print(margin, y, zSTRING(statBuf));
        y += fontH + fontH;

        // ── Keybindings help ──
        viewPanel->SetFontColor(zCOLOR(140, 140, 140));

        viewPanel->Print(margin, y, zSTRING("F9  - Heatmap"));  y += fontH;
        viewPanel->Print(margin, y, zSTRING("F10 - Panel"));     y += fontH;
        viewPanel->Print(margin, y, zSTRING("Home/End - Filter")); y += fontH;

        // ── Version ──
        viewPanel->SetFontColor(zCOLOR(100, 100, 100));
        zSTRING ver = "1.0.0";
        viewPanel->Print(8192 - viewPanel->FontSize(ver) - margin, y, ver);

        viewPanel->Blit();
        screen->RemoveItem(viewPanel);
    }

    // ================================================================
    //  FRAME HOOKS
    // ================================================================
    void TrailMap::GamePreLoop() {
        if (!enabled || !mapActive) return;
        HandleInput();
    }

    void TrailMap::GameLoop() {
        if (!enabled) return;

        // Record player position periodically
        RecordPosition();

        // Detect if the map is currently open
        if (!ogame || !ogame->GetGameWorld()) return;
        if (ogame->singleStep || ogame->pause_screen) return;

        bool mapOpen = DetectMapDocument();

        // Also try CoM's Ikarus sprite map
        if (!mapOpen && !mapActive) {
            CoMHack();
        }

        // Check if we need to deactivate
        if (mapActive) {
            if (hook == TM_CoM) {
                // For CoM: check if sprite handles went to 0 (map was closed)
                bool comStillOpen = false;
                if (indexSpriteMapHandle != Invalid && indexSpriteCursorHandle != Invalid) {
                    int smh = parser->GetSymbol(indexSpriteMapHandle)->single_intdata;
                    int sch = parser->GetSymbol(indexSpriteCursorHandle)->single_intdata;
                    comStillOpen = (smh != 0 && sch != 0);
                }
                if (!comStillOpen) {
                    DeactivateOverlay();
                }
            }
            else if (!mapOpen) {
                // For normal/NoMap: deactivate when document is gone
                DeactivateOverlay();
            }
        }
    }

    void TrailMap::GamePostLoop() {
        if (!enabled || !mapActive) return;
        if (ogame->singleStep || ogame->pause_screen) return;

        RenderHeatmap();
        RenderPanel();

        // Restore viewport (same as ItemMap)
        int sx, sy, ssx, ssy;
        screen->GetViewport(sx, sy, ssx, ssy);
        zrenderer->SetViewport(sx, sy, ssx, ssy);
    }
}

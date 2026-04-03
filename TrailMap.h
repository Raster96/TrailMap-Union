// TrailMap - Heatmap overlay for Gothic 2 map
// Union HEADER file

#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

namespace GOTHIC_ENGINE {

    // ── Filter ──────────────────────────────────────────────
    enum TrailFilter {
        TF_ALL = 0,
        TF_CURRENT,
        TF_CHAPTER_1,
        TF_CHAPTER_2,
        TF_CHAPTER_3,
        TF_CHAPTER_4,
        TF_CHAPTER_5,
        TF_CHAPTER_6,
        TF_CHAPTER_7,
        TF_CHAPTER_8,
        TF_CHAPTER_9,
        TF_NONE
    };

    static const int TF_MAX = TF_NONE;

    static const char* TrailFilterNames[] = {
        "All",
        "Current",
        "Chapter 1",
        "Chapter 2",
        "Chapter 3",
        "Chapter 4",
        "Chapter 5",
        "Chapter 6",
        "Chapter 7",
        "Chapter 8",
        "Chapter 9",
        "Off"
    };

    // ── Grid cell ───────────────────────────────────────────
    struct CellKey {
        int gx, gz;
        bool operator<(const CellKey& o) const {
            return gx != o.gx ? gx < o.gx : gz < o.gz;
        }
    };

    struct CellData {
        std::map<int, int> chapterVisits;   // chapter -> visit count

        int TotalCount() const {
            int t = 0;
            for (std::map<int,int>::const_iterator it = chapterVisits.begin(); it != chapterVisits.end(); ++it) {
                t += it->second;
            }
            return t;
        }
        int CountFor(int ch) const {
            std::map<int,int>::const_iterator it = chapterVisits.find(ch);
            return it != chapterVisits.end() ? it->second : 0;
        }
    };

    // ── Hook type ───────────────────────────────────────────
    enum TM_HookType {
        TM_Normal,
        TM_CoM,
        TM_NoMap,
        TM_NoHook
    };

    // ── Main class ──────────────────────────────────────────
    class TrailMap {
    public:
        TrailMap();
        ~TrailMap();

        // Life-cycle
        void Init();
        void ReadOptions();
        void RecordPosition();
        void SaveData(int slotId);
        void LoadData(int slotId);
        void ClearAllData();

        // Frame hooks
        void GameLoop();
        void GamePostLoop();
        void GamePreLoop();

        // CoM (Chronicles of Myrtana) Ikarus map support
        void CoMHack();
        int indexSpriteMapHandle;
        int indexSpriteCursorHandle;

        // Map document detection (public for hook access)
        bool TryInitFromDoc(oCViewDocument* doc);

        // Settings
        bool   enabled;
        int    gridSize;
        int    maxVisitsForColor;
        int    saveSlot;
        int    stepCounts[10];  // Per-chapter step counts (index 1-9)

        TrailFilter filter;

        // Trail data :  worldKey  ->  (cell -> data)
        std::map<std::string, std::map<CellKey, CellData> > data;

    private:
        // Map overlay state
        bool        mapActive;
        bool        showHeatmap;
        bool        showPanel;
        bool        transparentPanel;
        TM_HookType hook;
        int         mapRotate;

        zVEC4 mapCoords;
        zVEC4 worldCoords;
        int   markX, markY;

        // Timing
        std::chrono::steady_clock::time_point lastInitTime;
        zVEC3 lastPlayerPos;
        CellKey lastCellKey;  // Track last visited cell to avoid duplicate visits

        // Views
        zCView* viewDot;
        zCView* viewPanel;

        // ── Internal helpers ────────────────────────────────
        bool DetectMapDocument();
        void ActivateOverlay(TM_HookType hookType, int rotate = 0);
        void DeactivateOverlay(bool clearKeys = false);
        void HandleInput();

        void RenderHeatmap();
        void RenderPanel();

        zPOS   WorldToScreen(float wx, float wz);
        zCOLOR GetHeatColor(int visits);
        int    GetCurrentChapter();
        bool   IsCoMActive();
        int    GetEffectiveGridSize();
        std::string GetWorldKey();
        int    GetFilteredCount(const CellData& cell);
    };

    extern std::unique_ptr<TrailMap> trailMap;
}

META
{
  Parser = Menu;
  After = zUnionMenu.d;
  Namespace = TrailMap;
};

// Namespace = TrailMap
// File encoding: UTF-8 (without BOM).

// ------ Constants ------
const int Start_PY  = 1400;
const int Title_PY  = 450;
const int Menu_DY   = 550;
// Text
const int Text_PX   = 400;
const int Text_SX   = 8000;
const int Text_SY   = 750;
const int Text_DY   = 120;
// Choice
const int Choice_PX = 6400;
const int Choice_SX = 1500;
const int Choice_SY = 350;
const int Choice_DY = 120;

const string MenuBackPic   = "UnionMenu_BackPic.tga";
const string ItemBackPic   = "";
const string ChoiceBackPic = "MENU_CHOICE_BACK.TGA";
const string FontSmall     = "font_old_10_white.tga";
const string FontBig       = "font_old_20_white.tga";

var int CurrentMenuItem_PY;

// ------ Prototypes ------
func void InitializeBackPicturesAndFonts()
{
  MenuBackPic   = MENU_BACK_PIC;
  ItemBackPic   = MENU_ITEM_BACK_PIC;
  ChoiceBackPic = MENU_CHOICE_BACK_PIC;
  FontSmall     = MENU_FONT_SMALL;
  FontBig       = MENU_FONT_DEFAULT;
};

prototype C_EMPTY_MENU_DEF(C_MENU)
{
  InitializeBackPicturesAndFonts();
  C_MENU_DEF();
  backpic    = MenuBackPic;
  items[0]   = "";
  items[100] = "Union_menuitem_back";
  flags      = flags | MENU_SHOW_INFO;
};

instance C_MENU_ITEM_TEXT_BASE(C_MENU_ITEM_DEF)
{
  backpic        = ItemBackPic;
  posx           = Text_PX;
  posy           = Start_PY;
  dimx           = Text_SX;
  dimy           = Text_SY;
  flags          = flags | IT_EFFECTS_NEXT;
  onselaction[0] = SEL_ACTION_UNDEF;
};

instance C_MENUITEM_CHOICE_BASE(C_MENU_ITEM_DEF)
{
  backpic  = ChoiceBackPic;
  type     = MENU_ITEM_CHOICEBOX;
  fontname = FontSmall;
  posx     = Choice_PX;
  posy     = Start_PY + Choice_DY;
  dimx     = Choice_SX;
  dimy     = Choice_SY;
  flags    = flags & ~IT_SELECTABLE;
  flags    = flags | IT_TXT_CENTER;
};

instance MenuItem_Opt_Headline(C_MENU_ITEM_DEF)
{
  type    = MENU_ITEM_TEXT;
  posx    = 0;
  posy    = Title_PY;
  dimx    = 8100;
  flags   = flags & ~IT_SELECTABLE;
  flags   = flags | IT_TXT_CENTER;
  text[0] = Str_GetLocalizedString(
    "TrailMap НАСТРОЙКИ",
    "TrailMap SETTINGS",
    "TrailMap EINSTELLUNGEN",
    "USTAWIENIA TrailMap"
  );
};

func int Act_OpenWebLink()
{
  Open_Link("https://github.com/Raster96/TrailMap-Union");
  return 0;
};

instance MenuItem_Opt_Open_Link(C_MENU_ITEM_DEF)
{
  C_MENU_ITEM_TEXT_BASE();
  posy += MENU_DY * 8;

  posx             = 64;
  onselaction[0]   = SEL_ACTION_UNDEF;
  oneventaction[1] = Act_OpenWebLink;
  flags            = flags | IT_TXT_CENTER;
  text[0]          = Str_GetLocalizedString(
    "Открыть страницу проекта",
    "Open project page",
    "Projektseite öffnen",
    "Otwórz stronę projektu"
  );

  text[1]          = "https://github.com/Raster96/TrailMap-Union";
};

// ------ Menu ------
instance MenuItem_Union_Auto_TrailMap(C_MENU_ITEM_UNION_DEF)
{
  text[0]          = "TrailMap";
  text[1] = Str_GetLocalizedString(
    "Настройте параметры TrailMap",
    "Configure TrailMap settings",
    "TrailMap-Einstellungen konfigurieren",
    "Zmień ustawienia TrailMap"
  );
  onselaction[0]   = SEL_ACTION_STARTMENU;
  onselaction_s[0] = "TrailMap:Menu_Opt_TrailMap";
};

instance Menu_Opt_TrailMap(C_EMPTY_MENU_DEF)
{
  Menu_SearchItems("TrailMap:MENUITEM_OPT_TRAILMAP_*");
};

instance MenuItem_Opt_TrailMap_Headline(C_MENU_ITEM)
{
  MenuItem_Opt_Headline();
};

// ====== 1. Enabled (On/Off) ======
instance MenuItem_Opt_TrailMap_Enabled(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 1;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = "Enabled";
  text[1] = Str_GetLocalizedString(
    "Включить или отключить плагин",
    "Enable or disable the plugin",
    "Plugin aktivieren oder deaktivieren",
    "Włącz lub wyłącz plugin"
  );
};

instance MenuItem_Opt_TrailMap_Enabled_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "Enabled";
  onchgsetoptionsection = "TRAILMAP";
  text[0]               = Str_GetLocalizedString(
    "Выкл.|Вкл.",
    "Off|On",
    "Aus|Ein",
    "Wył.|Wł."
  );
};

// ====== 2. MaxVisitsForColor ======
instance MenuItem_Opt_TrailMap_MaxVisits(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 2;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = "MaxVisitsForColor";
  text[1] = Str_GetLocalizedString(
    "Макс. посещений для темного цвета",
    "Max visits for darkest color",
    "Max. Besuche für dunkelste Farbe",
    "Maks. wizyt dla najciemniejszego koloru"
  );
};

instance MenuItem_Opt_TrailMap_MaxVisits_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "MaxVisitsForColor";
  onchgsetoptionsection = "TRAILMAP";
  text[0]               = "1|3|5|10|20|50|100";
};

// ====== 3. ShowHeatmap (On/Off) ======
instance MenuItem_Opt_TrailMap_ShowHeatmap(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 3;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = "ShowHeatmap";
  text[1] = Str_GetLocalizedString(
    "Показывать тепловую карту",
    "Show heatmap overlay",
    "Heatmap-Overlay anzeigen",
    "Pokaż heatmapę na mapie"
  );
};

instance MenuItem_Opt_TrailMap_ShowHeatmap_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "ShowHeatmap";
  onchgsetoptionsection = "TRAILMAP";
  text[0]               = Str_GetLocalizedString(
    "Выкл.|Вкл.",
    "Off|On",
    "Aus|Ein",
    "Wył.|Wł."
  );
};

// ====== 4. ShowPanel (On/Off) ======
instance MenuItem_Opt_TrailMap_ShowPanel(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 4;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = "ShowPanel";
  text[1] = Str_GetLocalizedString(
    "Показывать панель фильтров",
    "Show filter panel",
    "Filterpanel anzeigen",
    "Pokaż panel filtrów"
  );
};

instance MenuItem_Opt_TrailMap_ShowPanel_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "ShowPanel";
  onchgsetoptionsection = "TRAILMAP";
  text[0]               = Str_GetLocalizedString(
    "Выкл.|Вкл.",
    "Off|On",
    "Aus|Ein",
    "Wył.|Wł."
  );
};

// ====== 5. TransparentPanel (On/Off) ======
instance MenuItem_Opt_TrailMap_TransparentPanel(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 5;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = "TransparentPanel";
  text[1] = Str_GetLocalizedString(
    "Прозрачная панель",
    "Transparent panel",
    "Transparentes Panel",
    "Przezroczysty panel"
  );
};

instance MenuItem_Opt_TrailMap_TransparentPanel_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "TransparentPanel";
  onchgsetoptionsection = "TRAILMAP";
  text[0]               = Str_GetLocalizedString(
    "Выкл.|Вкл.",
    "Off|On",
    "Aus|Ein",
    "Wył.|Wł."
  );
};

// ====== 6. Open Project Page ======
instance MenuItem_Opt_TRAILMAP_Open_Link(C_MENU_ITEM)
{
  MenuItem_Opt_Open_Link();
};

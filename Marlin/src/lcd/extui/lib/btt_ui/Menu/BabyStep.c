#include "BabyStep.h"
#include "../includes.h"

#define ITEM_BABYSTEP_UNIT_NUM 3

const ITEM itemBabyStepUnit[ITEM_BABYSTEP_UNIT_NUM] = {
  // icon                           label
  {ICON_001_MM,                     LABEL_001_MM},
  {ICON_01_MM,                      LABEL_01_MM},
  {ICON_1_MM,                       LABEL_1_MM},
};

const float babystep_unit[ITEM_BABYSTEP_UNIT_NUM] = {0.01f, 0.1f, 1};
static uint8_t   curUnit = 0;

static float baby_step_value = BABYSTEP_DEFAULT_VALUE;
static float orig_z_offset;

/* Reset to default */
void babyStepReset(void)
{
  baby_step_value = BABYSTEP_DEFAULT_VALUE;
}

/* Set default offset */
void babySetDefaultOffset(void)
{
  float last_unit;
  float processed_baby_step = 0.0f;

  int8_t neg = 1;
  if (baby_step_value < 0.0f)
    neg = -1;

  int stepcount = (baby_step_value * neg) / BABYSTEP_MAX_UNIT;
  for (; stepcount > 0; stepcount--)
  {
    MarlinAPI_BabyStep(-(BABYSTEP_MAX_UNIT * neg));
    // mustStoreCmd("M290 Z%.2f\n", -(BABYSTEP_MAX_UNIT * neg));
    processed_baby_step += BABYSTEP_MAX_UNIT;
  }

  last_unit = (baby_step_value * neg) - processed_baby_step;
  if (last_unit > 0.0f)
  {
    MarlinAPI_BabyStep(-(last_unit * neg));
    // mustStoreCmd("M290 Z%.2f\n", -(last_unit * neg));
    processed_baby_step += last_unit;
  }
  baby_step_value -= (processed_baby_step * neg);
}

void babyStepReDraw(bool skip_header)
{
  if (!skip_header)
  {
    GUI_DispString(exhibitRect.x0, exhibitRect.y0, textSelect(LABEL_BABYSTEP));
    GUI_DispString(exhibitRect.x0, exhibitRect.y0 + BYTE_HEIGHT + LARGE_BYTE_HEIGHT, textSelect(LABEL_Z_OFFSET));
  }

  char tempstr[20];

  GUI_POINT point_bs = {exhibitRect.x1, exhibitRect.y0 + BYTE_HEIGHT};
  GUI_POINT point_of = {exhibitRect.x1, exhibitRect.y0 + BYTE_HEIGHT*2 + LARGE_BYTE_HEIGHT};

  setLargeFont(true);

  sprintf(tempstr, "% 6.2f", baby_step_value);
  GUI_DispStringRight(point_bs.x, point_bs.y, (uint8_t *)tempstr);

  sprintf(tempstr, "% 6.2f", orig_z_offset);
  GUI_DispStringRight(point_of.x, point_of.y, (uint8_t *)tempstr);

  setLargeFont(false);
}

MENUITEMS babyStepItems = {
  // title
  LABEL_BABYSTEP,
  // icon                         label
  {{ICON_DEC,                     LABEL_DEC},
    {ICON_BACKGROUND,              LABEL_BACKGROUND},
    {ICON_BACKGROUND,              LABEL_BACKGROUND},
    {ICON_INC,                     LABEL_INC},
    {ICON_BACKGROUND,              LABEL_BACKGROUND},
    {ICON_001_MM,                  LABEL_001_MM},
    {ICON_RESET_VALUE,             LABEL_RESET},
    {ICON_BACK,                    LABEL_BACK},}
};
  // 1 title, ITEM_PER_PAGE items (icon + label)

static float max_unit;
static float now;

void menuBabyStepCallBack(void)
{
  KEY_VALUES key_num = menuKeyGetValue();
  switch (key_num)
  {
    // decrease babystep / z-offset
    case KEY_ICON_0:
      if (baby_step_value > BABYSTEP_MIN_VALUE)
      {
        float diff = baby_step_value - BABYSTEP_MIN_VALUE;
        max_unit = (diff > max_unit) ? max_unit : diff;

        MarlinAPI_BabyStep(-max_unit);
        // mustStoreCmd("M290 Z-%.2f\n", max_unit);

        baby_step_value -= max_unit;
        orig_z_offset -= max_unit;
      }
      break;

    // increase babystep / z-offset
    case KEY_ICON_3:
      if (baby_step_value < BABYSTEP_MAX_VALUE)
      {
        float diff = BABYSTEP_MAX_VALUE - baby_step_value;
        max_unit = (diff > max_unit) ? max_unit : diff;

        MarlinAPI_BabyStep(max_unit);
        // mustStoreCmd("M290 Z%.2f\n", max_unit);

        baby_step_value += max_unit;
        orig_z_offset += max_unit;
      }
      break;

    // save to EEPROM and apply Z offset
    case KEY_ICON_4:
      if (infoMachineSettings.EEPROM == 1)
      {
        showDialog(DIALOG_TYPE_QUESTION, textSelect(babyStepItems.title.index), textSelect(LABEL_EEPROM_SAVE_INFO),
          textSelect(LABEL_CONFIRM), textSelect(LABEL_CANCEL), saveEepromSettings, NULL, NULL);
      }
      break;

    // change step unit
    case KEY_ICON_5:
      curUnit = (curUnit + 1) % ITEM_BABYSTEP_UNIT_NUM;
      max_unit = babystep_unit[curUnit];
      babyStepItems.items[key_num] = itemBabyStepUnit[curUnit];
      
      menuDrawItem(&babyStepItems.items[key_num], key_num);

      break;

    // reset babystep to default value
    case KEY_ICON_6:
      if (baby_step_value != BABYSTEP_DEFAULT_VALUE)
      {
        babySetDefaultOffset();
        orig_z_offset = getParameter(P_PROBE_OFFSET, Z_STEPPER);
      }
      break;

    case KEY_ICON_7:
      infoMenu.cur--;
      break;

    default:
      max_unit = babystep_unit[curUnit];
      #if LCD_ENCODER_SUPPORT
        if (encoderPosition)
        {
          MarlinAPI_BabyStep(babystep_unit[curUnit] * encoderPosition);
          // mustStoreCmd("M290 Z%.2f\n", babystep_unit[curUnit] * encoderPosition);

          baby_step_value += babystep_unit[curUnit] * encoderPosition;
          encoderPosition = 0;
        }
      #endif
      break;
  }

  if (now != baby_step_value)
  {
    now = baby_step_value;
    babyStepReDraw(true);
    MarlinApi_SetParameter(P_PROBE_OFFSET, Z_STEPPER, orig_z_offset);
  }
  loopFrontEnd();
}

void menuBabyStep(void)
{
  baby_step_value = MarlinAPI_BabyStepTotal();
  #if FRIENDLY_PROBE_OFFSET_LANGUAGE == 1
    babyStepItems.items[0].icon = ICON_NOZZLE_DOWN;
    babyStepItems.items[0].label.index = LABEL_DOWN;
    babyStepItems.items[3].icon = ICON_NOZZLE_UP;
    babyStepItems.items[3].label.index = LABEL_UP;
  #endif

  orig_z_offset = getParameter(P_PROBE_OFFSET, Z_STEPPER);

  //char buf[64];
  //char dbl_str[10];
  //char dbl_str2[10];
  //dtostrf(orig_z_offset, 1, 2, dbl_str);
  //dtostrf(baby_step_value, 1, 2, dbl_str2);
  //sprintf(buf,"orig Z off is %s, Baby step is %s \n", dbl_str, dbl_str2);
  //addToast(DIALOG_TYPE_INFO, buf);

  //storeCmd(buf);

  if (infoMachineSettings.EEPROM == 1)
  {
    babyStepItems.items[KEY_ICON_4].icon = ICON_EEPROM_SAVE;
    babyStepItems.items[KEY_ICON_4].label.index = LABEL_SAVE;
  }

  babyStepItems.items[KEY_ICON_5] = itemBabyStepUnit[curUnit];

  menuDrawPage(&babyStepItems);
  babyStepReDraw(false);

#if LCD_ENCODER_SUPPORT
  encoderPosition = 0;
#endif

  max_unit = babystep_unit[curUnit];
  now = baby_step_value;
  menuSetFrontCallBack(menuBabyStepCallBack);
}

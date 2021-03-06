/* Copyright 2018-2019 eswai <@eswai>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "nicola.h"

#if !defined(__AVR__)
  #include <string.h>
  #define memcpy_P(des, src, len) memcpy(des, src, len)
#endif

#define NGBUFFER 5 // バッファのサイズ

static uint8_t ng_chrcount = 0; // 文字キー入力のカウンタ (シフトキー含む)
static bool is_nicola = false; // 親指シフトがオンかオフか
static uint8_t nicola_layer = 0; // レイヤー番号
static uint8_t n_modifier = 0; // 押しているmodifierキーの数
static uint64_t keycomb = 0UL; // 同時押しの状態を示す。64bitの各ビットがキーに対応する。

// 31キーを64bitの各ビットに割り当てる
#define B_Q    ((uint64_t)1<<0)
#define B_W    ((uint64_t)1<<1)
#define B_E    ((uint64_t)1<<2)
#define B_R    ((uint64_t)1<<3)
#define B_T    ((uint64_t)1<<4)

#define B_Y    ((uint64_t)1<<5)
#define B_U    ((uint64_t)1<<6)
#define B_I    ((uint64_t)1<<7)
#define B_O    ((uint64_t)1<<8)
#define B_P    ((uint64_t)1<<9)

#define B_A    ((uint64_t)1<<10)
#define B_S    ((uint64_t)1<<11)
#define B_D    ((uint64_t)1<<12)
#define B_F    ((uint64_t)1<<13)
#define B_G    ((uint64_t)1<<14)

#define B_H    ((uint64_t)1<<15)
#define B_J    ((uint64_t)1<<16)
#define B_K    ((uint64_t)1<<17)
#define B_L    ((uint64_t)1<<18)
#define B_SCLN ((uint64_t)1<<19)

#define B_Z    ((uint64_t)1<<20)
#define B_X    ((uint64_t)1<<21)
#define B_C    ((uint64_t)1<<22)
#define B_V    ((uint64_t)1<<23)
#define B_B    ((uint64_t)1<<24)

#define B_N    ((uint64_t)1<<25)
#define B_M    ((uint64_t)1<<26)
#define B_COMM ((uint64_t)1<<27)
#define B_DOT  ((uint64_t)1<<28)
#define B_SLSH ((uint64_t)1<<29)

#define B_1    ((uint64_t)1<<30)
#define B_2    ((uint64_t)1<<31)
#define B_3    ((uint64_t)1<<32)
#define B_4    ((uint64_t)1<<33)
#define B_5    ((uint64_t)1<<34)

#define B_6    ((uint64_t)1<<35)
#define B_7    ((uint64_t)1<<36)
#define B_8    ((uint64_t)1<<37)
#define B_9    ((uint64_t)1<<38)
#define B_0    ((uint64_t)1<<39)

#define B_SHFTL ((uint64_t)1<<40)
#define B_SHFTR ((uint64_t)1<<41)

// 文字入力バッファ
static uint16_t ninputs[NGBUFFER];

// キーコードとキービットの対応
// メモリ削減のため配列はNG_Qを0にしている
const uint64_t ng_key[] = {
  [NG_Q    - NG_Q] = B_Q,
  [NG_W    - NG_Q] = B_W,
  [NG_E    - NG_Q] = B_E,
  [NG_R    - NG_Q] = B_R,
  [NG_T    - NG_Q] = B_T,

  [NG_Y    - NG_Q] = B_Y,
  [NG_U    - NG_Q] = B_U,
  [NG_I    - NG_Q] = B_I,
  [NG_O    - NG_Q] = B_O,
  [NG_P    - NG_Q] = B_P,

  [NG_A    - NG_Q] = B_A,
  [NG_S    - NG_Q] = B_S,
  [NG_D    - NG_Q] = B_D,
  [NG_F    - NG_Q] = B_F,
  [NG_G    - NG_Q] = B_G,

  [NG_H    - NG_Q] = B_H,
  [NG_J    - NG_Q] = B_J,
  [NG_K    - NG_Q] = B_K,
  [NG_L    - NG_Q] = B_L,
  [NG_SCLN - NG_Q] = B_SCLN,

  [NG_Z    - NG_Q] = B_Z,
  [NG_X    - NG_Q] = B_X,
  [NG_C    - NG_Q] = B_C,
  [NG_V    - NG_Q] = B_V,
  [NG_B    - NG_Q] = B_B,

  [NG_N    - NG_Q] = B_N,
  [NG_M    - NG_Q] = B_M,
  [NG_COMM - NG_Q] = B_COMM,
  [NG_DOT  - NG_Q] = B_DOT,
  [NG_SLSH - NG_Q] = B_SLSH,
  
  [NG_1 - NG_Q] = B_1,
  [NG_2 - NG_Q] = B_2,
  [NG_3 - NG_Q] = B_3,
  [NG_4 - NG_Q] = B_4,
  [NG_5 - NG_Q] = B_5,
  
  [NG_6 - NG_Q] = B_6,
  [NG_7 - NG_Q] = B_7,
  [NG_8 - NG_Q] = B_8,
  [NG_9 - NG_Q] = B_9,
  [NG_0 - NG_Q] = B_0,

  [NG_SHFTL - NG_Q] = B_SHFTL,
  [NG_SHFTR - NG_Q] = B_SHFTR,
};

// 親指シフトカナ変換テーブル
typedef struct {
  uint64_t key;
  char kana[4];
} nicola_keymap;

const PROGMEM nicola_keymap ngmap[] = {
  // 単独
  {.key = B_Q               , .kana = "["},
  {.key = B_W               , .kana = "]"},
  {.key = B_E               , .kana = "ho"},
  {.key = B_R               , .kana = "bi"},
  {.key = B_T               , .kana = "-"},
  {.key = B_Y               , .kana = "("},
  {.key = B_U               , .kana = ")"},
  {.key = B_I               , .kana = "to"},
  {.key = B_O               , .kana = "ha"},
  {.key = B_P               , .kana = "po"},

  {.key = B_A               , .kana = "ki"},
  {.key = B_S               , .kana = "si"},
  {.key = B_D               , .kana = "u"},
  {.key = B_F               , .kana = "te"},
  {.key = B_G               , .kana = "gi"},
  {.key = B_H               , .kana = "yu"},
  {.key = B_J               , .kana = "nn"},
  {.key = B_K               , .kana = "i"},
  {.key = B_L               , .kana = "ka"},
  {.key = B_SCLN            , .kana = "ta"},

  {.key = B_Z               , .kana = "ji"},
  {.key = B_X               , .kana = "ti"},
  {.key = B_C               , .kana = "ni"},
  {.key = B_V               , .kana = "ri"},
  {.key = B_B               , .kana = "bu"},
  {.key = B_N               , .kana = "xya"},
  {.key = B_M               , .kana = "xtu"},
  {.key = B_COMM            , .kana = "xyo"},
  {.key = B_DOT             , .kana = "xyu"},
  {.key = B_SLSH            , .kana = "sa"},

  {.key = B_1               , .kana = "1"},
  {.key = B_2               , .kana = "2"},
  {.key = B_3               , .kana = "3"},
  {.key = B_4               , .kana = "4"},
  {.key = B_5               , .kana = "5"},
  {.key = B_6               , .kana = "6"},
  {.key = B_7               , .kana = "7"},
  {.key = B_8               , .kana = "8"},
  {.key = B_9               , .kana = "9"},
  {.key = B_0               , .kana = "0"},

  // Shift and space
  // {.key = B_SHFTL           , .kana = " "},
  // {.key = B_SHFTR           , .kana = " "},

  // Shift and Henkan/Muhenkan
  // {.key = B_SHFTL           , .kana = SS_TAP(X_INT5)},
  // {.key = B_SHFTR           , .kana = SS_TAP(X_INT4)},

  // 左シフト
  {.key = B_SHFTL|B_Q       , .kana = "xi"},
  {.key = B_SHFTL|B_W       , .kana = "hi"},
  {.key = B_SHFTL|B_E       , .kana = "ke"},
  {.key = B_SHFTL|B_R       , .kana = "xa"},
  {.key = B_SHFTL|B_T       , .kana = "xu"},
  {.key = B_SHFTL|B_Y       , .kana = "vu"},
  {.key = B_SHFTL|B_U       , .kana = "ge"},
  {.key = B_SHFTL|B_I       , .kana = "yo"},
  {.key = B_SHFTL|B_O       , .kana = "hu"},
  {.key = B_SHFTL|B_P       , .kana = "he"},

  {.key = B_SHFTL|B_A       , .kana = "da"},
  {.key = B_SHFTL|B_S       , .kana = "a"},
  {.key = B_SHFTL|B_D       , .kana = "ga"},
  {.key = B_SHFTL|B_F       , .kana = "ba"},
  {.key = B_SHFTL|B_G       , .kana = "xe"},
  {.key = B_SHFTL|B_H       , .kana = "zu"},
  {.key = B_SHFTL|B_J       , .kana = "ru"},
  {.key = B_SHFTL|B_K       , .kana = "su"},
  {.key = B_SHFTL|B_L       , .kana = "ma"},
  {.key = B_SHFTL|B_SCLN    , .kana = "de"},

  {.key = B_SHFTL|B_Z       , .kana = "ze"},
  {.key = B_SHFTL|B_X       , .kana = "ne"},
  {.key = B_SHFTL|B_C       , .kana = "se"},
  {.key = B_SHFTL|B_V       , .kana = "pi"},
  {.key = B_SHFTL|B_B       , .kana = "xo"},
  {.key = B_SHFTL|B_N       , .kana = "ya"},
  {.key = B_SHFTL|B_M       , .kana = "e"},
  {.key = B_SHFTL|B_COMM    , .kana = ","},
  {.key = B_SHFTL|B_DOT     , .kana = "."},
  {.key = B_SHFTL|B_SLSH    , .kana = "?"},

  {.key = B_SHFTL|B_1       , .kana = "$"},
  {.key = B_SHFTL|B_2       , .kana = "@"},
  {.key = B_SHFTL|B_3       , .kana = "#"},
  {.key = B_SHFTL|B_4       , .kana = "%"},
  {.key = B_SHFTL|B_5       , .kana = "'"},
  {.key = B_SHFTL|B_6       , .kana = "\""},
  {.key = B_SHFTL|B_7       , .kana = "_"},
  {.key = B_SHFTL|B_8       , .kana = "="},
  {.key = B_SHFTL|B_9       , .kana = "+"},
  {.key = B_SHFTL|B_0       , .kana = "~"},

  // 右シフト
  {.key = B_SHFTR|B_Q       , .kana = "ro"},
  {.key = B_SHFTR|B_W       , .kana = "be"},
  {.key = B_SHFTR|B_E       , .kana = "re"},
  {.key = B_SHFTR|B_R       , .kana = "pe"},
  {.key = B_SHFTR|B_T       , .kana = "go"},
  {.key = B_SHFTR|B_Y       , .kana = "di"},
  {.key = B_SHFTR|B_U       , .kana = "nu"},
  {.key = B_SHFTR|B_I       , .kana = "do"},
  {.key = B_SHFTR|B_O       , .kana = "me"},
  {.key = B_SHFTR|B_P       , .kana = "zo"},

  {.key = B_SHFTR|B_A       , .kana = "wa"},
  {.key = B_SHFTR|B_S       , .kana = "o"},
  {.key = B_SHFTR|B_D       , .kana = "na"},
  {.key = B_SHFTR|B_F       , .kana = "ra"},
  {.key = B_SHFTR|B_G       , .kana = "pu"},
  {.key = B_SHFTR|B_H       , .kana = "du"},
  {.key = B_SHFTR|B_J       , .kana = "ku"},
  {.key = B_SHFTR|B_K       , .kana = "no"},
  {.key = B_SHFTR|B_L       , .kana = "ko"},
  {.key = B_SHFTR|B_SCLN    , .kana = "so"},

  {.key = B_SHFTR|B_Z       , .kana = "pa"},
  {.key = B_SHFTR|B_X       , .kana = "gu"},
  {.key = B_SHFTR|B_C       , .kana = "mi"},
  {.key = B_SHFTR|B_V       , .kana = "za"},
  {.key = B_SHFTR|B_B       , .kana = "bo"},
  {.key = B_SHFTR|B_N       , .kana = "mu"},
  {.key = B_SHFTR|B_M       , .kana = "wo"},
  {.key = B_SHFTR|B_COMM    , .kana = "tu"},
  {.key = B_SHFTR|B_DOT     , .kana = "mo"},
  {.key = B_SHFTR|B_SLSH    , .kana = "!"},

  {.key = B_SHFTR|B_1       , .kana = "/"},
  {.key = B_SHFTR|B_2       , .kana = "|"},
  {.key = B_SHFTR|B_3       , .kana = "\\"},
  {.key = B_SHFTR|B_4       , .kana = "`"},
  {.key = B_SHFTR|B_5       , .kana = ""},
  {.key = B_SHFTR|B_6       , .kana = "<"},
  {.key = B_SHFTR|B_7       , .kana = ">"},
  {.key = B_SHFTR|B_8       , .kana = "*"},
  {.key = B_SHFTR|B_9       , .kana = "^"},
  {.key = B_SHFTR|B_0       , .kana = "&"},
};

// 親指シフトのレイヤー、シフトキーを設定
void set_nicola(uint8_t layer) {
  nicola_layer = layer;
}

// 親指シフトをオンオフ
void nicola_on(void) {
  is_nicola = true;
  keycomb = 0UL;
  nicola_clear();
  layer_on(nicola_layer);

  tap_code(KC_LANG1); // Mac
  tap_code(KC_HENK); // Win
}

void nicola_off(void) {
  is_nicola = false;
  keycomb = 0UL;
  nicola_clear();
  layer_off(nicola_layer);

  tap_code(KC_LANG2); // Mac
  tap_code(KC_MHEN); // Win
}

// 親指シフトの状態
bool nicola_state(void) {
  return is_nicola;
}

// キー入力を文字に変換して出力する
void nicola_type(void) {
  nicola_keymap bngmap; // PROGMEM buffer

  bool douji = false; // 同時押しか連続押しか
  uint64_t skey = 0; // 連続押しの場合のバッファ

  switch (keycomb) {
    // send_stringできないキー、長すぎるマクロはここで定義
    // case B_F|B_G:
    //   nicola_off();
    //   break;
    default:
      // キーから仮名に変換して出力する。
      // 同時押しの場合 ngmapに定義されている
      for (int i = 0; i < sizeof ngmap / sizeof bngmap; i++) {
        memcpy_P(&bngmap, &ngmap[i], sizeof(bngmap));
        if (keycomb == bngmap.key) {
          douji = true;
          send_string(bngmap.kana);
          break;
        }
      }
      // 連続押しの場合 ngmapに定義されていない
      if (!douji) {
        for (int j = 0; j < ng_chrcount; j++) {
          skey = ng_key[ninputs[j] - NG_Q];
          if ((keycomb & B_SHFTL) > 0) skey |= B_SHFTL; // シフトキー状態を反映
          if ((keycomb & B_SHFTR) > 0) skey |= B_SHFTR; // シフトキー状態を反映
          for (int i = 0; i < sizeof ngmap / sizeof bngmap; i++) {
            memcpy_P(&bngmap, &ngmap[i], sizeof(bngmap));
            if (skey == bngmap.key) {
              send_string(bngmap.kana);
              break;
            }
          }
        }
      }
  }

  nicola_clear(); // バッファを空にする
}

// バッファをクリアする
void nicola_clear(void) {
  for (int i = 0; i < NGBUFFER; i++) {
    ninputs[i] = 0;
  }
  ng_chrcount = 0;
  #ifndef NICOLA_RENZOKU // 連続シフト、シフト押しっぱなしで入力可能
    keycomb = 0UL;
  #endif
}

// 入力モードか編集モードかを確認する
void nicola_mode(uint16_t keycode, keyrecord_t *record) {
  if (!is_nicola) return;

  // modifierが押されたらレイヤーをオフ
  switch (keycode) {
    case KC_LCTRL:
    case KC_LSHIFT:
    case KC_LALT:
    case KC_LGUI:
    case KC_RCTRL:
    case KC_RSHIFT:
    case KC_RALT:
    case KC_RGUI:
      if (record->event.pressed) {
        n_modifier++;
        layer_off(nicola_layer);
      } else {
        n_modifier--;
        if (n_modifier == 0) {
          layer_on(nicola_layer);
        }
      }
      break;
  }

}

// 親指シフトの入力処理
bool process_nicola(uint16_t keycode, keyrecord_t *record) {
  // if (!is_nicola || n_modifier > 0) return true;

  if (record->event.pressed) {
    switch (keycode) {
      case NG_Q ... NG_SHFTR:
        ninputs[ng_chrcount] = keycode; // キー入力をバッファに貯める
        ng_chrcount++;
        keycomb |= ng_key[keycode - NG_Q]; // キーの重ね合わせ
        // 2文字押したら処理を開始
        if (ng_chrcount > 1) {
          nicola_type();
        }
        return false;
        break;
    }
  } else { // key release
    switch (keycode) {
      case NG_Q ... NG_SHFTR:
        // 2文字入力していなくても、どれかキーを離したら処理を開始する
        if (ng_chrcount > 0) {
          nicola_type();
        }
        keycomb &= ~ng_key[keycode - NG_Q]; // キーの重ね合わせ
        return false;
        break;
    }
  }
  return true;
}

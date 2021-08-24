// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "soc/i2s_periph.h"
#include "soc/gpio_sig_map.h"

/*
 Bunch of constants for every I2S peripheral: GPIO signals, irqs, hw addr of registers etc
*/
const i2s_signal_conn_t i2s_periph_signal[SOC_I2S_NUM] = {
    {
        .mck_out_sig  = I2S0_MCLK_OUT_IDX,
        .rx_bck_sig   = I2S0I_BCK_IN_IDX,
        .tx_bck_sig   = I2S0O_BCK_OUT_IDX,
        .tx_ws_sig    = I2S0O_WS_OUT_IDX,
        .rx_ws_sig    = I2S0I_WS_IN_IDX,
        .data_out_sig = I2S0O_SD_OUT_IDX,
        .data_in_sig  = I2S0I_SD_IN_IDX,
        .irq          = -1,
        .module       = PERIPH_I2S0_MODULE,
    },
    {
        .mck_out_sig  = I2S1_MCLK_OUT_IDX,
        .rx_bck_sig   = I2S1I_BCK_IN_IDX,
        .tx_bck_sig   = I2S1O_BCK_OUT_IDX,
        .tx_ws_sig    = I2S1O_WS_OUT_IDX,
        .rx_ws_sig    = I2S1I_WS_IN_IDX,
        .data_out_sig = I2S1O_SD_OUT_IDX,
        .data_in_sig  = I2S1I_SD_IN_IDX,
        .irq          = -1,
        .module       = PERIPH_I2S1_MODULE,
    }
};
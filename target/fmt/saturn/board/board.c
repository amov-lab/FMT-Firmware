/******************************************************************************
 * Copyright 2020 The Firmament Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include <firmament.h>

#include <board.h>
#include <board_device.h>
#include <shell.h>
#include <string.h>

#include "board_device.h"
#include "drv_systick.h"
#include "drv_usart.h"
#include "drv_usbd_cdc.h"
#include "drv_gpio.h"
#include "led.h"

#include "module/console/console_config.h"
#include "module/file_manager/file_manager.h"
#include "module/task_manager/task_manager.h"
#include "module/toml/toml.h"
#include "module/work_queue/workqueue_manager.h"

/*
#define DEFAULT_TOML_SYS_CONFIG "target = \"FMT Saturn\"\n\
[console]\n\
        [[console.devices]]\n\
        type = \"serial\"\n\
        name = \"serial0\"\n\
        baudrate = 57600\n\
        auto-switch = true\n\
        [[console.devices]]\n\
        type = \"mavlink\"\n\
        name = \"mav_console\"\n\
        auto-switch = true"
    */

// #define MATCH(a, b)     (strcmp(a, b) == 0)
// #define SYS_CONFIG_FILE "/sys/sysconfig.toml"

static const struct dfs_mount_tbl mnt_table[] = {
    // { "sd0", "/", "elm", 0, NULL },
    { NULL } /* NULL indicate the end */
};

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    console_printf("Enter Error_Handler\n");
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5) {
    }
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSE_Enable();

    /* Wait till HSE is ready */
    while (LL_RCC_HSE_IsReady() != 1) {
    }
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 336, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 336, LL_RCC_PLLQ_DIV_7);
    LL_RCC_PLL_Enable();

    /* Wait till PLL is ready */
    while (LL_RCC_PLL_IsReady() != 1) {
    }
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    }
    LL_SetSystemCoreClock(168000000);

    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
}

#define ITEM_LENGTH 42
void bsp_show_information(void)
{
    char buffer[50];

    console_printf("\n");
    console_println("   _____                               __ ");
    console_println("  / __(_)_____ _  ___ ___ _  ___ ___  / /_");
    console_println(" / _// / __/  ' \\/ _ `/  ' \\/ -_) _ \\/ __/");
    console_println("/_/ /_/_/ /_/_/_/\\_,_/_/_/_/\\__/_//_/\\__/ ");

    sprintf(buffer, "FMT FMU %s", FMT_VERSION);
    print_item_line("Firmware", buffer, '.', ITEM_LENGTH);
    sprintf(buffer, "RT-Thread v%ld.%ld.%ld", RT_VERSION, RT_SUBVERSION, RT_REVISION);
    print_item_line("Kernel", buffer, '.', ITEM_LENGTH);
    sprintf(buffer, "%d KB", SYSTEM_TOTAL_MEM_SIZE / 1024);
    print_item_line("RAM", buffer, '.', ITEM_LENGTH);
    print_item_line("Target", TARGET_NAME, '.', ITEM_LENGTH);
    print_item_line("Vehicle", VEHICLE_TYPE, '.', ITEM_LENGTH);
    //     print_item_line("INS Model", ins_model_info.info, '.', ITEM_LENGTH);
    //     print_item_line("FMS Model", fms_model_info.info, '.', ITEM_LENGTH);
    //     print_item_line("Control Model", control_model_info.info, '.', ITEM_LENGTH);
    // #ifdef FMT_USING_SIH
    //     print_item_line("Plant Model", plant_model_info.info, '.', ITEM_LENGTH);
    // #endif

    console_println("Task Initialize:");
    fmt_task_desc_t task_tab = get_task_table();
    for (uint32_t i = 0; i < get_task_num(); i++) {
        sprintf(buffer, "  %s", task_tab[i].name);
        /* task status must be okay to reach here */
        print_item_line(buffer, get_task_status(task_tab[i].name) == TASK_OK ? "OK" : "Fail", '.', ITEM_LENGTH);
    }
}

/* this function will be called before rtos start, which is not in the thread context */
void bsp_early_initialize(void)
{
    /* init system heap */
    rt_system_heap_init((void*)SYSTEM_FREE_MEM_BEGIN, (void*)SYSTEM_FREE_MEM_END);

    /* HAL library initialization */
    HAL_Init();

    /* System clock initialization */
    SystemClock_Config();

    /* usart driver init */
    RT_CHECK(drv_usart_init());

    /* init console to enable console output */
    FMT_CHECK(console_init());

    /* systick driver init */
    RT_CHECK(drv_systick_init());

    /* system time module init */
    FMT_CHECK(systime_init());

    /* gpio driver init */
    RT_CHECK(drv_gpio_init());

    /* system statistic module */
    FMT_CHECK(sys_stat_init());
}

/* this function will be called after rtos start, which is in thread context */
void bsp_initialize(void)
{
    /* init uMCN */
    FMT_CHECK(mcn_init());

    /* create workqueue */
    FMT_CHECK(workqueue_manager_init());

    file_manager_init(mnt_table);

    /* init usbd_cdc */
    RT_CHECK(drv_usb_cdc_init());

    /* init finsh */
    finsh_system_init();
    /* Mount finsh to console after finsh system init */
    FMT_CHECK(console_enable_input());
}

void bsp_post_initialize(void)
{
    /* toml system configure */
    // __toml_root_tab = toml_parse_config_file(SYS_CONFIG_FILE);
    // if (!__toml_root_tab) {
    //     /* use default system configuration */
    //     __toml_root_tab = toml_parse_config_string(DEFAULT_TOML_SYS_CONFIG);
    // }
    // FMT_CHECK(bsp_parse_toml_sysconfig(__toml_root_tab));

    /* initialize led */
    FMT_CHECK(led_control_init());

    /* show system information */
    bsp_show_information();
}

/**
 * This function will initial STM32 board.
 */
void rt_hw_board_init()
{
    bsp_early_initialize();
}

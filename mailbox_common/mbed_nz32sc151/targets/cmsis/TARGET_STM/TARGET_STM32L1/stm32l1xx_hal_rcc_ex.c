/**
  ******************************************************************************
  * @file    stm32l1xx_hal_rcc_ex.c
  * @author  MCD Application Team
  * @version V1.1.1
  * @date    31-March-2015
  * @brief   Extended RCC HAL module driver.
  *    
  *          This file provides firmware functions to manage the following 
  *          functionalities RCC extension peripheral:
  *           + Extended Peripheral Control functions
  *  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************  
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/** @addtogroup STM32L1xx_HAL_Driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED


/** @defgroup RCCEx RCCEx
  * @brief RCC Extension HAL module driver
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup RCCEx_Private_Constants RCCEx Private Constants
  * @{
  */
/**
  * @}
  */
  
/* Private macro -------------------------------------------------------------*/
/** @defgroup RCCEx_Private_Macros RCCEx Private Macros
  * @{
  */
/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Functions RCCEx Exported Functions
  * @{
  */

/** @defgroup RCCEx_Exported_Functions_Group1 Extended Peripheral Control functions 
 *  @brief  Extended Peripheral Control functions  
 *
@verbatim   
 ===============================================================================
                ##### Extended Peripheral Control functions  #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions allowing to control the RCC Clocks 
    frequencies.
    [..] 
    (@) Important note: Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to
        select the RTC clock source; in this case the Backup domain will be reset in  
        order to modify the RTC Clock source, as consequence RTC registers (including 
        the backup registers) and RCC_BDCR register are set to their reset values.
      
@endverbatim
  * @{
  */

/**
  * @brief  Initializes the RCC extended peripherals clocks according to the specified parameters in the
  *         RCC_PeriphCLKInitTypeDef.
  * @param  PeriphClkInit: pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         contains the configuration information for the Extended Peripherals clocks(RTC/LCD clock).
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t tickstart = 0;
  uint32_t tmp_reg = 0;
  
  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClkInit->PeriphClockSelection));
  
  /*------------------------------- RTC/LCD Configuration ------------------------*/ 
  if ((((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC) 
#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
      || (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LCD) == RCC_PERIPHCLK_LCD)
#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
    )
  {
    /* check for RTC Parameters used to output RTCCLK */
    if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC)
    {
      assert_param(IS_RCC_RTCCLKSOURCE(PeriphClkInit->RTCClockSelection));
    }

#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
    if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LCD) == RCC_PERIPHCLK_LCD)
    {
      assert_param(IS_RCC_RTCCLKSOURCE(PeriphClkInit->LCDClockSelection));
    }
#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

    /* Enable Power Controller clock */
    __HAL_RCC_PWR_CLK_ENABLE();
    
    /* Enable write access to Backup domain */
    SET_BIT(PWR->CR, PWR_CR_DBP);
    
    /* Wait for Backup domain Write protection disable */
    tickstart = HAL_GetTick();
    
    while((PWR->CR & PWR_CR_DBP) == RESET)
    {
      if((HAL_GetTick() - tickstart ) > RCC_DBP_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
    
    /* Reset the Backup domain only if the RTC Clock source selection is modified */ 
    tmp_reg = (RCC->CSR & RCC_CSR_RTCSEL);
    
    if(((tmp_reg != (PeriphClkInit->RTCClockSelection & RCC_CSR_RTCSEL)) \
      && (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC))
#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
      || ((tmp_reg != (PeriphClkInit->LCDClockSelection & RCC_CSR_RTCSEL)) \
       && (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LCD) == RCC_PERIPHCLK_LCD))
#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
     )
    {
      /* Store the content of CSR register before the reset of Backup Domain */
      tmp_reg = (RCC->CSR & ~(RCC_CSR_RTCSEL));
      
      /* RTC Clock selection can be changed only if the Backup Domain is reset */
      __HAL_RCC_BACKUPRESET_FORCE();
      __HAL_RCC_BACKUPRESET_RELEASE();
      
      /* Restore the Content of CSR register */
      RCC->CSR = tmp_reg;
    }
    
    /* If LSE is selected as RTC clock source, wait for LSE reactivation */
    if ((PeriphClkInit->RTCClockSelection == RCC_RTCCLKSOURCE_LSE)
#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
      || (PeriphClkInit->LCDClockSelection == RCC_RTCCLKSOURCE_LSE)
#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
      )
    {
      /* Get timeout */   
      tickstart = HAL_GetTick();
      
      /* Wait till LSE is ready */  
      while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
      {
        if((HAL_GetTick() - tickstart ) > RCC_LSE_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
    
    __HAL_RCC_RTC_CONFIG(PeriphClkInit->RTCClockSelection);
  }
  
  return HAL_OK;
}

/**
  * @brief  Get the PeriphClkInit according to the internal
  * RCC configuration registers.
  * @param  PeriphClkInit: pointer to an RCC_PeriphCLKInitTypeDef structure that 
  *         returns the configuration information for the Extended Peripherals clocks(RTC/LCD clocks).
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t srcclk = 0;
  
  /* Set all possible values for the extended clock type parameter------------*/
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_RTC;
#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_LCD;
#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

  /* Get the RTC/LCD configuration -----------------------------------------------*/
  srcclk = __HAL_RCC_GET_RTC_SOURCE();
  if (srcclk != RCC_RTCCLKSOURCE_HSE_DIV2)
  {
    /* Source clock is LSE or LSI*/
    PeriphClkInit->RTCClockSelection = srcclk;
  }
  else
  {
    /* Source clock is HSE. Need to get the prescaler value*/
    PeriphClkInit->RTCClockSelection = srcclk | (READ_BIT(RCC->CR, RCC_CR_RTCPRE));
  }
#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
  PeriphClkInit->LCDClockSelection = PeriphClkInit->RTCClockSelection;
#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
}

/**
  * @brief  Returns the peripheral clock frequency
  * @note   Returns 0 if peripheral clock is unknown
  * @param  PeriphClk: Peripheral clock identifier
  *         This parameter can be one of the following values:
  *            @arg RCC_PERIPHCLK_RTC:  RTC peripheral clock
  *            @arg RCC_PERIPHCLK_LCD:  LCD peripheral clock (depends on devices)
  * @retval Frequency in Hz (0: means that no available frequency for the peripheral)
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{
  uint32_t tmp_reg = 0, frequency = 0;
  uint32_t srcclk = 0;

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClk));
  
  switch (PeriphClk)
  {
  case RCC_PERIPHCLK_RTC:
#if defined(STM32L100xB) || defined(STM32L100xBA) || defined(STM32L100xC)\
 || defined(STM32L152xB) || defined(STM32L152xBA) || defined(STM32L152xC)\
 || defined(STM32L162xC) || defined(STM32L152xCA) || defined(STM32L152xD)\
 || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L152xE) || defined(STM32L152xDX)\
 || defined(STM32L162xE) || defined(STM32L162xDX)
  case RCC_PERIPHCLK_LCD:
#endif /* STM32L100xB || STM32L152xBA || ... || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
    {
      /* Get RCC CSR configuration ------------------------------------------------------*/
      tmp_reg = RCC->CSR;

      /* Get the current RTC source */
      srcclk = __HAL_RCC_GET_RTC_SOURCE();

      /* Check if LSE is ready if RTC clock selection is LSE */
      if ((srcclk == RCC_RTCCLKSOURCE_LSE) && (HAL_IS_BIT_SET(tmp_reg, RCC_CSR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Check if LSI is ready if RTC clock selection is LSI */
      else if ((srcclk == RCC_RTCCLKSOURCE_LSI) && (HAL_IS_BIT_SET(tmp_reg, RCC_CSR_LSIRDY)))
      {
        frequency = LSI_VALUE;
      }
      /* Check if HSE is ready */
      else if (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSERDY))
      {
        switch (READ_BIT(RCC->CR, RCC_CR_RTCPRE))
        {
          case RCC_CR_RTCPRE:     /* HSE DIV16 has been selected */
          {
            frequency = HSE_VALUE / 16;
            break;
          }
          case RCC_CR_RTCPRE_1:   /* HSE DIV8 has been selected */
          {
            frequency = HSE_VALUE / 8;
            break;
          }
          case RCC_CR_RTCPRE_0:   /* HSE DIV4 has been selected */
          {
            frequency = HSE_VALUE / 4;
            break;
          }
          default:
          {
            frequency = HSE_VALUE / 2;
            break;
          }
        }
      }
      /* Clock not enabled for RTC*/
      else
      {
        frequency = 0;
      }
      break;
    }
  default: 
    {
      break;
    }
  }
  return(frequency);
}

#if  (defined(STM32L100xBA) || defined(STM32L151xBA) || defined(STM32L152xBA) || defined(STM32L100xC) || defined(STM32L151xC) || defined(STM32L152xC) || defined(STM32L162xC) || defined(STM32L151xCA) || defined(STM32L151xD) || defined(STM32L152xCA) || defined(STM32L152xD) || defined(STM32L162xCA) || defined(STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined(STM32L152xE) || defined(STM32L152xDX) || defined(STM32L162xE) || defined(STM32L162xDX))

/**
  * @brief  Enables the LSE Clock Security System.
  * @note   If a failure is detected on the external 32 kHz oscillator, the LSE clock is no longer supplied
  *         to the RTC but no hardware action is made to the registers.
  *         In Standby mode a wakeup is generated. In other modes an interrupt can be sent to wakeup
  *         the software (see Section 5.3.4: Clock interrupt register (RCC_CIR) on page 104).
  *         The software MUST then disable the LSECSSON bit, stop the defective 32 kHz oscillator
  *         (disabling LSEON), and can change the RTC clock source (no clock or LSI or HSE, with
  *         RTCSEL), or take any required action to secure the application.  
  * @note   LSE CSS available only for high density and medium+ devices
  * @retval None
  */
void HAL_RCCEx_EnableLSECSS(void)
{
  *(__IO uint32_t *) CSR_LSECSSON_BB = (uint32_t)ENABLE;
}

/**
  * @brief  Disables the LSE Clock Security System.
  * @note   Once enabled this bit cannot be disabled, except after an LSE failure detection 
  *         (LSECSSD=1). In that case the software MUST disable the LSECSSON bit.
  *         Reset by power on reset and RTC software reset (RTCRST bit).
  * @note   LSE CSS available only for high density and medium+ devices
  * @retval None
  */
void HAL_RCCEx_DisableLSECSS(void)
{
  *(__IO uint32_t *) CSR_LSECSSON_BB = (uint32_t)DISABLE;
}
#endif /* STM32L100xBA || STM32L151xBA || STM32L152xBA || STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
  
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_RCC_MODULE_ENABLED */
/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

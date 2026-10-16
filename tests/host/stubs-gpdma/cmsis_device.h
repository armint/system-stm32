#pragma once

// Host stand-in for the GPDMA part of stm32h533xx.h: the register layout and
// the DMA_C* bit definitions are copied from that file (cmsis_device_h5
// v1.7.0). GPDMA1_BASE only has to exist; the test builds its own register
// image and passes it to Dma::Channel.

#include <cstdint>

#define __IO volatile

typedef struct
{
  __IO uint32_t SECCFGR;     /*!< DMA secure configuration register,               Address offset: 0x00  */
  __IO uint32_t PRIVCFGR;    /*!< DMA privileged configuration register,           Address offset: 0x04  */
  __IO uint32_t RCFGLOCKR;   /*!< DMA lock configuration register,                 Address offset: 0x08  */
  __IO uint32_t MISR;        /*!< DMA non secure masked interrupt status register, Address offset: 0x0C  */
  __IO uint32_t SMISR;       /*!< DMA secure masked interrupt status register,     Address offset: 0x10  */
} DMA_TypeDef;
typedef struct
{
  __IO uint32_t CLBAR;        /*!< DMA channel x linked-list base address register, Address offset: 0x50 + (x * 0x80) */
       uint32_t RESERVED1[2]; /*!< Reserved 1,                                      Address offset: 0x54 -- 0x58      */
  __IO uint32_t CFCR;         /*!< DMA channel x flag clear register,               Address offset: 0x5C + (x * 0x80) */
  __IO uint32_t CSR;          /*!< DMA channel x flag status register,              Address offset: 0x60 + (x * 0x80) */
  __IO uint32_t CCR;          /*!< DMA channel x control register,                  Address offset: 0x64 + (x * 0x80) */
       uint32_t RESERVED2[10];/*!< Reserved 2,                                      Address offset: 0x68 -- 0x8C      */
  __IO uint32_t CTR1;         /*!< DMA channel x transfer register 1,               Address offset: 0x90 + (x * 0x80) */
  __IO uint32_t CTR2;         /*!< DMA channel x transfer register 2,               Address offset: 0x94 + (x * 0x80) */
  __IO uint32_t CBR1;         /*!< DMA channel x block register 1,                  Address offset: 0x98 + (x * 0x80) */
  __IO uint32_t CSAR;         /*!< DMA channel x source address register,           Address offset: 0x9C + (x * 0x80) */
  __IO uint32_t CDAR;         /*!< DMA channel x destination address register,      Address offset: 0xA0 + (x * 0x80) */
  __IO uint32_t CTR3;         /*!< DMA channel x transfer register 3,               Address offset: 0xA4 + (x * 0x80) */
  __IO uint32_t CBR2;         /*!< DMA channel x block register 2,                  Address offset: 0xA8 + (x * 0x80) */
       uint32_t RESERVED3[8]; /*!< Reserved 3,                                      Address offset: 0xAC -- 0xC8      */
  __IO uint32_t CLLR;         /*!< DMA channel x linked-list address register,      Address offset: 0xCC + (x * 0x80) */
} DMA_Channel_TypeDef;

#define GPDMA1_BASE 0x40020000UL

#define DMA_CLBAR_LBA_Pos                   (16U)
#define DMA_CLBAR_LBA_Msk                   (0xFFFFUL << DMA_CLBAR_LBA_Pos)         /*!< 0xFFFF0000 */
#define DMA_CLBAR_LBA                       DMA_CLBAR_LBA_Msk                       /*!< Linked-list Base Address of DMA channel x */
#define DMA_CFCR_TCF_Pos                    (8U)
#define DMA_CFCR_TCF_Msk                    (0x1UL << DMA_CFCR_TCF_Pos)             /*!< 0x00000100 */
#define DMA_CFCR_TCF                        DMA_CFCR_TCF_Msk                        /*!< Transfer complete flag clear             */
#define DMA_CFCR_HTF_Pos                    (9U)
#define DMA_CFCR_HTF_Msk                    (0x1UL << DMA_CFCR_HTF_Pos)             /*!< 0x00000200 */
#define DMA_CFCR_HTF                        DMA_CFCR_HTF_Msk                        /*!< Half transfer complete flag clear        */
#define DMA_CFCR_DTEF_Pos                   (10U)
#define DMA_CFCR_DTEF_Msk                   (0x1UL << DMA_CFCR_DTEF_Pos)            /*!< 0x00000400 */
#define DMA_CFCR_DTEF                       DMA_CFCR_DTEF_Msk                       /*!< Data transfer error flag clear           */
#define DMA_CFCR_ULEF_Pos                   (11U)
#define DMA_CFCR_ULEF_Msk                   (0x1UL << DMA_CFCR_ULEF_Pos)            /*!< 0x00000800 */
#define DMA_CFCR_ULEF                       DMA_CFCR_ULEF_Msk                       /*!< Update linked-list item error flag clear */
#define DMA_CFCR_USEF_Pos                   (12U)
#define DMA_CFCR_USEF_Msk                   (0x1UL << DMA_CFCR_USEF_Pos)            /*!< 0x00001000 */
#define DMA_CFCR_USEF                       DMA_CFCR_USEF_Msk                       /*!< User setting error flag clear            */
#define DMA_CFCR_SUSPF_Pos                  (13U)
#define DMA_CFCR_SUSPF_Msk                  (0x1UL << DMA_CFCR_SUSPF_Pos)           /*!< 0x00002000 */
#define DMA_CFCR_SUSPF                      DMA_CFCR_SUSPF_Msk                      /*!< Completed suspension flag clear          */
#define DMA_CFCR_TOF_Pos                    (14U)
#define DMA_CFCR_TOF_Msk                    (0x1UL << DMA_CFCR_TOF_Pos)             /*!< 0x00004000 */
#define DMA_CFCR_TOF                        DMA_CFCR_TOF_Msk                        /*!< Trigger overrun flag clear               */
#define DMA_CSR_IDLEF_Pos                   (0U)
#define DMA_CSR_IDLEF_Msk                   (0x1UL << DMA_CSR_IDLEF_Pos)            /*!< 0x00000001 */
#define DMA_CSR_IDLEF                       DMA_CSR_IDLEF_Msk                       /*!< Idle flag                          */
#define DMA_CSR_TCF_Pos                     (8U)
#define DMA_CSR_TCF_Msk                     (0x1UL << DMA_CSR_TCF_Pos)              /*!< 0x00000100 */
#define DMA_CSR_TCF                         DMA_CSR_TCF_Msk                         /*!< Transfer complete flag             */
#define DMA_CSR_HTF_Pos                     (9U)
#define DMA_CSR_HTF_Msk                     (0x1UL << DMA_CSR_HTF_Pos)              /*!< 0x00000200 */
#define DMA_CSR_HTF                         DMA_CSR_HTF_Msk                         /*!< Half transfer complete flag        */
#define DMA_CSR_DTEF_Pos                    (10U)
#define DMA_CSR_DTEF_Msk                    (0x1UL << DMA_CSR_DTEF_Pos)             /*!< 0x00000400 */
#define DMA_CSR_DTEF                        DMA_CSR_DTEF_Msk                        /*!< Data transfer error flag           */
#define DMA_CSR_ULEF_Pos                    (11U)
#define DMA_CSR_ULEF_Msk                    (0x1UL << DMA_CSR_ULEF_Pos)             /*!< 0x00000800 */
#define DMA_CSR_ULEF                        DMA_CSR_ULEF_Msk                        /*!< Update linked-list item error flag */
#define DMA_CSR_USEF_Pos                    (12U)
#define DMA_CSR_USEF_Msk                    (0x1UL << DMA_CSR_USEF_Pos)             /*!< 0x00001000 */
#define DMA_CSR_USEF                        DMA_CSR_USEF_Msk                        /*!< User setting error flag            */
#define DMA_CSR_SUSPF_Pos                   (13U)
#define DMA_CSR_SUSPF_Msk                   (0x1UL << DMA_CSR_SUSPF_Pos)            /*!< 0x00002000 */
#define DMA_CSR_SUSPF                       DMA_CSR_SUSPF_Msk                       /*!< User setting error flag            */
#define DMA_CSR_TOF_Pos                     (14U)
#define DMA_CSR_TOF_Msk                     (0x1UL << DMA_CSR_TOF_Pos)              /*!< 0x00004000 */
#define DMA_CSR_TOF                         DMA_CSR_TOF_Msk                         /*!< Trigger overrun flag               */
#define DMA_CSR_FIFOL_Pos                   (16U)
#define DMA_CSR_FIFOL_Msk                   (0xFFUL << DMA_CSR_FIFOL_Pos)           /*!< 0x00FF0000 */
#define DMA_CSR_FIFOL                       DMA_CSR_FIFOL_Msk                       /*!< Monitored FIFO level in bytes      */
#define DMA_CCR_EN_Pos                      (0U)
#define DMA_CCR_EN_Msk                      (0x1UL << DMA_CCR_EN_Pos)               /*!< 0x00000001 */
#define DMA_CCR_EN                          DMA_CCR_EN_Msk                          /*!< Channel enable                                 */
#define DMA_CCR_RESET_Pos                   (1U)
#define DMA_CCR_RESET_Msk                   (0x1UL << DMA_CCR_RESET_Pos)            /*!< 0x00000002 */
#define DMA_CCR_RESET                       DMA_CCR_RESET_Msk                       /*!< Channel reset                                  */
#define DMA_CCR_SUSP_Pos                    (2U)
#define DMA_CCR_SUSP_Msk                    (0x1UL << DMA_CCR_SUSP_Pos)             /*!< 0x00000004 */
#define DMA_CCR_SUSP                        DMA_CCR_SUSP_Msk                        /*!< Channel suspend                                */
#define DMA_CCR_TCIE_Pos                    (8U)
#define DMA_CCR_TCIE_Msk                    (0x1UL << DMA_CCR_TCIE_Pos)             /*!< 0x00000100 */
#define DMA_CCR_TCIE                        DMA_CCR_TCIE_Msk                        /*!< Transfer complete interrupt enable             */
#define DMA_CCR_HTIE_Pos                    (9U)
#define DMA_CCR_HTIE_Msk                    (0x1UL << DMA_CCR_HTIE_Pos)             /*!< 0x00000200 */
#define DMA_CCR_HTIE                        DMA_CCR_HTIE_Msk                        /*!< Half transfer complete interrupt enable        */
#define DMA_CCR_DTEIE_Pos                   (10U)
#define DMA_CCR_DTEIE_Msk                   (0x1UL << DMA_CCR_DTEIE_Pos)            /*!< 0x00000400 */
#define DMA_CCR_DTEIE                       DMA_CCR_DTEIE_Msk                       /*!< Data transfer error interrupt enable           */
#define DMA_CCR_ULEIE_Pos                   (11U)
#define DMA_CCR_ULEIE_Msk                   (0x1UL << DMA_CCR_ULEIE_Pos)            /*!< 0x00000800 */
#define DMA_CCR_ULEIE                       DMA_CCR_ULEIE_Msk                       /*!< Update linked-list item error interrupt enable */
#define DMA_CCR_USEIE_Pos                   (12U)
#define DMA_CCR_USEIE_Msk                   (0x1UL << DMA_CCR_USEIE_Pos)            /*!< 0x00001000 */
#define DMA_CCR_USEIE                       DMA_CCR_USEIE_Msk                       /*!< User setting error interrupt enable            */
#define DMA_CCR_SUSPIE_Pos                  (13U)
#define DMA_CCR_SUSPIE_Msk                  (0x1UL << DMA_CCR_SUSPIE_Pos)           /*!< 0x00002000 */
#define DMA_CCR_SUSPIE                      DMA_CCR_SUSPIE_Msk                      /*!< Completed suspension interrupt enable          */
#define DMA_CCR_TOIE_Pos                    (14U)
#define DMA_CCR_TOIE_Msk                    (0x1UL << DMA_CCR_TOIE_Pos)             /*!< 0x00004000 */
#define DMA_CCR_TOIE                        DMA_CCR_TOIE_Msk                        /*!< Trigger overrun interrupt enable               */
#define DMA_CCR_LSM_Pos                     (16U)
#define DMA_CCR_LSM_Msk                     (0x1UL << DMA_CCR_LSM_Pos)              /*!< 0x00010000 */
#define DMA_CCR_LSM                         DMA_CCR_LSM_Msk                         /*!< Link step mode                                 */
#define DMA_CCR_LAP_Pos                     (17U)
#define DMA_CCR_LAP_Msk                     (0x1UL << DMA_CCR_LAP_Pos)              /*!< 0x00020000 */
#define DMA_CCR_LAP                         DMA_CCR_LAP_Msk                         /*!< Linked-list allocated port                     */
#define DMA_CCR_PRIO_Pos                    (22U)
#define DMA_CCR_PRIO_Msk                    (0x3UL << DMA_CCR_PRIO_Pos)             /*!< 0x00C00000 */
#define DMA_CCR_PRIO                        DMA_CCR_PRIO_Msk                        /*!< Priority level                                 */
#define DMA_CCR_PRIO_0                      (0x1UL << DMA_CCR_PRIO_Pos)             /*!< 0x00400000 */
#define DMA_CCR_PRIO_1                      (0x2UL << DMA_CCR_PRIO_Pos)             /*!< 0x00800000 */
#define DMA_CTR1_SDW_LOG2_Pos               (0U)
#define DMA_CTR1_SDW_LOG2_Msk               (0x3UL << DMA_CTR1_SDW_LOG2_Pos)        /*!< 0x00000003 */
#define DMA_CTR1_SDW_LOG2                   DMA_CTR1_SDW_LOG2_Msk                   /*!< Binary logarithm of the source data width of a burst                    */
#define DMA_CTR1_SDW_LOG2_0                 (0x1UL << DMA_CTR1_SDW_LOG2_Pos)        /*!< Bit 0 */
#define DMA_CTR1_SDW_LOG2_1                 (0x2UL << DMA_CTR1_SDW_LOG2_Pos)        /*!< Bit 1 */
#define DMA_CTR1_SINC_Pos                   (3U)
#define DMA_CTR1_SINC_Msk                   (0x1UL << DMA_CTR1_SINC_Pos)            /*!< 0x00000008 */
#define DMA_CTR1_SINC                       DMA_CTR1_SINC_Msk                       /*!< Source incrementing burst                                               */
#define DMA_CTR1_SBL_1_Pos                  (4U)
#define DMA_CTR1_SBL_1_Msk                  (0x3FUL << DMA_CTR1_SBL_1_Pos)          /*!< 0x000003F0 */
#define DMA_CTR1_SBL_1                      DMA_CTR1_SBL_1_Msk                      /*!< Source burst length minus 1                                             */
#define DMA_CTR1_PAM_Pos                    (11U)
#define DMA_CTR1_PAM_Msk                    (0x3UL << DMA_CTR1_PAM_Pos)             /*!< 0x0001800 */
#define DMA_CTR1_PAM                        DMA_CTR1_PAM_Msk                        /*!< Padding / alignment mode                                                */
#define DMA_CTR1_PAM_0                      (0x1UL << DMA_CTR1_PAM_Pos)             /*!< Bit 0 */
#define DMA_CTR1_PAM_1                      (0x2UL << DMA_CTR1_PAM_Pos)             /*!< Bit 1 */
#define DMA_CTR1_SBX_Pos                    (13U)
#define DMA_CTR1_SBX_Msk                    (0x1UL << DMA_CTR1_SBX_Pos)             /*!< 0x00002000 */
#define DMA_CTR1_SBX                        DMA_CTR1_SBX_Msk                        /*!< Source byte exchange within the unaligned half-word of each source word */
#define DMA_CTR1_SAP_Pos                    (14U)
#define DMA_CTR1_SAP_Msk                    (0x1UL << DMA_CTR1_SAP_Pos)             /*!< 0x00004000 */
#define DMA_CTR1_SAP                        DMA_CTR1_SAP_Msk                        /*!< Source allocated port                                                   */
#define DMA_CTR1_SSEC_Pos                   (15U)
#define DMA_CTR1_SSEC_Msk                   (0x1UL << DMA_CTR1_SSEC_Pos)            /*!< 0x00008000 */
#define DMA_CTR1_SSEC                       DMA_CTR1_SSEC_Msk                       /*!< Security attribute of the DMA transfer from the source                  */
#define DMA_CTR1_DDW_LOG2_Pos               (16U)
#define DMA_CTR1_DDW_LOG2_Msk               (0x3UL << DMA_CTR1_DDW_LOG2_Pos)        /*!< 0x00030000 */
#define DMA_CTR1_DDW_LOG2                   DMA_CTR1_DDW_LOG2_Msk                   /*!< Binary logarithm of the destination data width of a burst               */
#define DMA_CTR1_DDW_LOG2_0                 (0x1UL << DMA_CTR1_DDW_LOG2_Pos)        /*!< Bit 0 */
#define DMA_CTR1_DDW_LOG2_1                 (0x2UL << DMA_CTR1_DDW_LOG2_Pos)        /*!< Bit 1 */
#define DMA_CTR1_DINC_Pos                   (19U)
#define DMA_CTR1_DINC_Msk                   (0x1UL << DMA_CTR1_DINC_Pos)            /*!< 0x00080000 */
#define DMA_CTR1_DINC                       DMA_CTR1_DINC_Msk                       /*!< Destination incrementing burst                                          */
#define DMA_CTR1_DBL_1_Pos                  (20U)
#define DMA_CTR1_DBL_1_Msk                  (0x3FUL << DMA_CTR1_DBL_1_Pos)          /*!< 0x03F00000 */
#define DMA_CTR1_DBL_1                      DMA_CTR1_DBL_1_Msk                      /*!< Destination burst length minus 1                                        */
#define DMA_CTR1_DBX_Pos                    (26U)
#define DMA_CTR1_DBX_Msk                    (0x1UL << DMA_CTR1_DBX_Pos)             /*!< 0x04000000 */
#define DMA_CTR1_DBX                        DMA_CTR1_DBX_Msk                        /*!< Destination byte exchange                                               */
#define DMA_CTR1_DHX_Pos                    (27U)
#define DMA_CTR1_DHX_Msk                    (0x1UL << DMA_CTR1_DHX_Pos)             /*!< 0x08000000 */
#define DMA_CTR1_DHX                        DMA_CTR1_DHX_Msk                        /*!< Destination half-word exchange                                          */
#define DMA_CTR1_DAP_Pos                    (30U)
#define DMA_CTR1_DAP_Msk                    (0x1UL << DMA_CTR1_DAP_Pos)             /*!< 0x40000000 */
#define DMA_CTR1_DAP                        DMA_CTR1_DAP_Msk                        /*!< Destination allocated port                                              */
#define DMA_CTR1_DSEC_Pos                   (31U)
#define DMA_CTR1_DSEC_Msk                   (0x1UL << DMA_CTR1_DSEC_Pos)            /*!< 0x80000000 */
#define DMA_CTR1_DSEC                       DMA_CTR1_DSEC_Msk                       /*!< Security attribute of the DMA transfer from the destination             */
#define DMA_CTR2_REQSEL_Pos                 (0U)
#define DMA_CTR2_REQSEL_Msk                 (0xFFUL << DMA_CTR2_REQSEL_Pos)         /*!< 0x000000FF */
#define DMA_CTR2_REQSEL                     DMA_CTR2_REQSEL_Msk                     /*!< DMA hardware request selection */
#define DMA_CTR2_SWREQ_Pos                  (9U)
#define DMA_CTR2_SWREQ_Msk                  (0x1UL << DMA_CTR2_SWREQ_Pos)           /*!< 0x00000200 */
#define DMA_CTR2_SWREQ                      DMA_CTR2_SWREQ_Msk                      /*!< Software request               */
#define DMA_CTR2_DREQ_Pos                   (10U)
#define DMA_CTR2_DREQ_Msk                   (0x1UL << DMA_CTR2_DREQ_Pos)            /*!< 0x00000400 */
#define DMA_CTR2_DREQ                       DMA_CTR2_DREQ_Msk                       /*!< Destination hardware request   */
#define DMA_CTR2_BREQ_Pos                   (11U)
#define DMA_CTR2_BREQ_Msk                   (0x1UL << DMA_CTR2_BREQ_Pos)            /*!< 0x00000800 */
#define DMA_CTR2_BREQ                       DMA_CTR2_BREQ_Msk                       /*!< Block hardware request         */
#define DMA_CTR2_PFREQ_Pos                  (12U)
#define DMA_CTR2_PFREQ_Msk                  (0x1UL << DMA_CTR2_PFREQ_Pos)           /*!< 0x00001000 */
#define DMA_CTR2_PFREQ                      DMA_CTR2_PFREQ_Msk                      /*!< Block hardware request */
#define DMA_CTR2_TRIGM_Pos                  (14U)
#define DMA_CTR2_TRIGM_Msk                  (0x3UL << DMA_CTR2_TRIGM_Pos)           /*!< 0x0000C000 */
#define DMA_CTR2_TRIGM                      DMA_CTR2_TRIGM_Msk                      /*!< Trigger mode                   */
#define DMA_CTR2_TRIGM_0                    (0x1UL << DMA_CTR2_TRIGM_Pos)           /*!< Bit 0 */
#define DMA_CTR2_TRIGM_1                    (0x2UL << DMA_CTR2_TRIGM_Pos)           /*!< Bit 1 */
#define DMA_CTR2_TRIGSEL_Pos                (16U)
#define DMA_CTR2_TRIGSEL_Msk                (0x3FUL << DMA_CTR2_TRIGSEL_Pos)        /*!< 0x003F0000 */
#define DMA_CTR2_TRIGSEL                    DMA_CTR2_TRIGSEL_Msk                    /*!< Trigger event input selection  */
#define DMA_CTR2_TRIGPOL_Pos                (24U)
#define DMA_CTR2_TRIGPOL_Msk                (0x3UL << DMA_CTR2_TRIGPOL_Pos)         /*!< 0x03000000 */
#define DMA_CTR2_TRIGPOL                    DMA_CTR2_TRIGPOL_Msk                    /*!< Trigger event polarity         */
#define DMA_CTR2_TRIGPOL_0                  (0x1UL << DMA_CTR2_TRIGPOL_Pos)         /*!< Bit 0 */
#define DMA_CTR2_TRIGPOL_1                  (0x2UL << DMA_CTR2_TRIGPOL_Pos)         /*!< Bit 1 */
#define DMA_CTR2_TCEM_Pos                   (30U)
#define DMA_CTR2_TCEM_Msk                   (0x3UL << DMA_CTR2_TCEM_Pos)            /*!< 0xC0000000 */
#define DMA_CTR2_TCEM                       DMA_CTR2_TCEM_Msk                       /*!< Transfer complete event mode   */
#define DMA_CTR2_TCEM_0                     (0x1UL << DMA_CTR2_TCEM_Pos)            /*!< Bit 0 */
#define DMA_CTR2_TCEM_1                     (0x2UL << DMA_CTR2_TCEM_Pos)            /*!< Bit 1 */
#define DMA_CBR1_BNDT_Pos                   (0U)
#define DMA_CBR1_BNDT_Msk                   (0xFFFFUL << DMA_CBR1_BNDT_Pos)         /*!< 0x0000FFFF */
#define DMA_CBR1_BNDT                       DMA_CBR1_BNDT_Msk                       /*!< Block number of data bytes to transfer from the source */
#define DMA_CBR1_BRC_Pos                    (16U)
#define DMA_CBR1_BRC_Msk                    (0x7FFUL << DMA_CBR1_BRC_Pos)           /*!< 0x07FF0000 */
#define DMA_CBR1_BRC                        DMA_CBR1_BRC_Msk                        /*!< Block repeat counter                                   */
#define DMA_CBR1_SDEC_Pos                   (28U)
#define DMA_CBR1_SDEC_Msk                   (0x1UL << DMA_CBR1_SDEC_Pos)            /*!< 0x10000000 */
#define DMA_CBR1_SDEC                       DMA_CBR1_SDEC_Msk                       /*!< Source address decrement                               */
#define DMA_CBR1_DDEC_Pos                   (29U)
#define DMA_CBR1_DDEC_Msk                   (0x1UL << DMA_CBR1_DDEC_Pos)            /*!< 0x20000000 */
#define DMA_CBR1_DDEC                       DMA_CBR1_DDEC_Msk                       /*!< Destination address decrement                          */
#define DMA_CBR1_BRSDEC_Pos                 (30U)
#define DMA_CBR1_BRSDEC_Msk                 (0x1UL << DMA_CBR1_BRSDEC_Pos)          /*!< 0x40000000 */
#define DMA_CBR1_BRSDEC                     DMA_CBR1_BRSDEC_Msk                     /*!< Block repeat source address decrement                  */
#define DMA_CBR1_BRDDEC_Pos                 (31U)
#define DMA_CBR1_BRDDEC_Msk                 (0x1UL << DMA_CBR1_BRDDEC_Pos)          /*!< 0x80000000 */
#define DMA_CBR1_BRDDEC                     DMA_CBR1_BRDDEC_Msk                     /*!< Block repeat destination address decrement             */
#define DMA_CSAR_SA_Pos                     (0U)
#define DMA_CSAR_SA_Msk                     (0xFFFFFFFFUL << DMA_CSAR_SA_Pos)       /*!< 0xFFFFFFFF */
#define DMA_CSAR_SA                         DMA_CSAR_SA_Msk                         /*!< Source Address */
#define DMA_CDAR_DA_Pos                     (0U)
#define DMA_CDAR_DA_Msk                     (0xFFFFFFFFUL << DMA_CDAR_DA_Pos)       /*!< 0xFFFFFFFF */
#define DMA_CDAR_DA                         DMA_CDAR_DA_Msk                         /*!< Destination address */
#define DMA_CLLR_LA_Pos                     (2U)
#define DMA_CLLR_LA_Msk                     (0x3FFFUL << DMA_CLLR_LA_Pos)           /*!< 0x0000FFFC */
#define DMA_CLLR_LA                         DMA_CLLR_LA_Msk                         /*!< Pointer to the next linked-list data structure */
#define DMA_CLLR_ULL_Pos                    (16U)
#define DMA_CLLR_ULL_Msk                    (0x1UL << DMA_CLLR_ULL_Pos)             /*!< 0x00010000 */
#define DMA_CLLR_ULL                        DMA_CLLR_ULL_Msk                        /*!< Update link address register from memory       */
#define DMA_CLLR_UB2_Pos                    (25U)
#define DMA_CLLR_UB2_Msk                    (0x1UL << DMA_CLLR_UB2_Pos)             /*!< 0x02000000 */
#define DMA_CLLR_UB2                        DMA_CLLR_UB2_Msk                        /*!< Update block register 2 from memory            */
#define DMA_CLLR_UT3_Pos                    (26U)
#define DMA_CLLR_UT3_Msk                    (0x1UL << DMA_CLLR_UT3_Pos)             /*!< 0x04000000 */
#define DMA_CLLR_UT3                        DMA_CLLR_UT3_Msk                        /*!< Update transfer register 3 from SRAM           */
#define DMA_CLLR_UDA_Pos                    (27U)
#define DMA_CLLR_UDA_Msk                    (0x1UL << DMA_CLLR_UDA_Pos)             /*!< 0x08000000 */
#define DMA_CLLR_UDA                        DMA_CLLR_UDA_Msk                        /*!< Update destination address register from SRAM  */
#define DMA_CLLR_USA_Pos                    (28U)
#define DMA_CLLR_USA_Msk                    (0x1UL << DMA_CLLR_USA_Pos)             /*!< 0x10000000 */
#define DMA_CLLR_USA                        DMA_CLLR_USA_Msk                        /*!< Update source address register from SRAM       */
#define DMA_CLLR_UB1_Pos                    (29U)
#define DMA_CLLR_UB1_Msk                    (0x1UL << DMA_CLLR_UB1_Pos)             /*!< 0x20000000 */
#define DMA_CLLR_UB1                        DMA_CLLR_UB1_Msk                        /*!< Update block register 1 from SRAM              */
#define DMA_CLLR_UT2_Pos                    (30U)
#define DMA_CLLR_UT2_Msk                    (0x1UL << DMA_CLLR_UT2_Pos)             /*!< 0x40000000 */
#define DMA_CLLR_UT2                        DMA_CLLR_UT2_Msk                        /*!< Update transfer register 2 from SRAM           */
#define DMA_CLLR_UT1_Pos                    (31U)
#define DMA_CLLR_UT1_Msk                    (0x1UL << DMA_CLLR_UT1_Pos)             /*!< 0x80000000 */
#define DMA_CLLR_UT1                        DMA_CLLR_UT1_Msk                        /*!< Update transfer register 1 from SRAM           */

// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
/**********************************************************************

    SGI MACE skeleton device

**********************************************************************/

#include "emu.h"
#include "mace.h"

#define LOG_READ_PCI        (1U << 0)
#define LOG_READ_VIN1       (1U << 1)
#define LOG_READ_VIN2       (1U << 2)
#define LOG_READ_VOUT       (1U << 3)
#define LOG_READ_ENET       (1U << 4)
#define LOG_READ_AUDIO      (1U << 5)
#define LOG_READ_ISA        (1U << 6)
#define LOG_READ_KBDMS      (1U << 7)
#define LOG_READ_I2C        (1U << 8)
#define LOG_READ_UST_MSC    (1U << 9)
#define LOG_READ_ISA_EXT    (1U << 10)
#define LOG_READ_RTC        (1U << 11)
#define LOG_WRITE_PCI       (1U << 12)
#define LOG_WRITE_VIN1      (1U << 13)
#define LOG_WRITE_VIN2      (1U << 14)
#define LOG_WRITE_VOUT      (1U << 15)
#define LOG_WRITE_ENET      (1U << 16)
#define LOG_WRITE_AUDIO     (1U << 17)
#define LOG_WRITE_ISA       (1U << 18)
#define LOG_WRITE_KBDMS     (1U << 19)
#define LOG_WRITE_I2C       (1U << 20)
#define LOG_WRITE_UST_MSC   (1U << 21)
#define LOG_WRITE_ISA_EXT   (1U << 22)
#define LOG_WRITE_RTC       (1U << 23)
#define LOG_HIFREQ          (1U << 24)
#define LOG_PCI             (LOG_READ_PCI     | LOG_WRITE_PCI)
#define LOG_VIN1            (LOG_READ_VIN1    | LOG_WRITE_VIN1)
#define LOG_VIN2            (LOG_READ_VIN2    | LOG_WRITE_VIN2)
#define LOG_VOUT            (LOG_READ_VOUT    | LOG_WRITE_VOUT)
#define LOG_ENET            (LOG_READ_ENET    | LOG_WRITE_ENET)
#define LOG_AUDIO           (LOG_READ_AUDIO   | LOG_WRITE_AUDIO)
#define LOG_ISA             (LOG_READ_ISA     | LOG_WRITE_ISA)
#define LOG_KBDMS           (LOG_READ_KBDMS   | LOG_WRITE_KBDMS)
#define LOG_UST_MSC         (LOG_READ_UST_MSC | LOG_WRITE_UST_MSC)
#define LOG_ISA_EXT         (LOG_READ_ISA_EXT | LOG_WRITE_ISA_EXT)
#define LOG_RTC             (LOG_READ_RTC     | LOG_WRITE_RTC)
#define LOG_ALL             (LOG_PCI | LOG_VIN1 | LOG_VIN2 | LOG_VOUT | LOG_ENET | LOG_AUDIO | LOG_ISA | LOG_KBDMS | LOG_UST_MSC | LOG_ISA_EXT | LOG_RTC)

#define VERBOSE             (LOG_ALL)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(SGI_MACE, mace_device, "sgimace", "SGI MACE")

mace_device::mace_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, SGI_MACE, tag, owner, clock)
	, m_maincpu(*this, finder_base::DUMMY_TAG)
	, m_rtc_read_callback(*this)
	, m_rtc_write_callback(*this)
{
}

void mace_device::device_resolve_objects()
{
	m_rtc_read_callback.resolve_safe(0);
	m_rtc_write_callback.resolve_safe();
}

void mace_device::device_start()
{
	save_item(NAME(m_isa.m_ringbase_reset));
	save_item(NAME(m_isa.m_flash_nic_ctrl));
	save_item(NAME(m_isa.m_int_status));
	save_item(NAME(m_isa.m_int_mask));

	save_item(NAME(m_ust_msc.m_msc));
	save_item(NAME(m_ust_msc.m_ust));
	save_item(NAME(m_ust_msc.m_ust_msc));
	save_item(NAME(m_ust_msc.m_compare1));
	save_item(NAME(m_ust_msc.m_compare2));
	save_item(NAME(m_ust_msc.m_compare3));
	save_item(NAME(m_ust_msc.m_ain_msc_ust));
	save_item(NAME(m_ust_msc.m_aout1_msc_ust));
	save_item(NAME(m_ust_msc.m_aout2_msc_ust));
	save_item(NAME(m_ust_msc.m_vin1_msc_ust));
	save_item(NAME(m_ust_msc.m_vin2_msc_ust));
	save_item(NAME(m_ust_msc.m_vout_msc_ust));

	m_timer_ust = timer_alloc(FUNC(mace_device::ust_tick), this);
	m_timer_msc = timer_alloc(FUNC(mace_device::msc_tick), this);
	m_timer_ust->adjust(attotime::never);
	m_timer_msc->adjust(attotime::never);
}

void mace_device::device_reset()
{
	memset(&m_isa, 0, sizeof(isa_t));
	memset(&m_ust_msc, 0, sizeof(ust_msc_t));

	m_timer_ust->adjust(attotime::from_nsec(960), 0, attotime::from_nsec(960));
	m_timer_msc->adjust(attotime::from_msec(1), 0, attotime::from_msec(1));
}

//**************************************************************************
//  DEVICE HARDWARE
//**************************************************************************

void mace_device::device_add_mconfig(machine_config &config)
{
}

void mace_device::map(address_map &map)
{
	map(0x00080000, 0x000fffff).rw(FUNC(mace_device::pci_r), FUNC(mace_device::pci_w));
	map(0x00100000, 0x0017ffff).rw(FUNC(mace_device::vin1_r), FUNC(mace_device::vin1_w));
	map(0x00180000, 0x001fffff).rw(FUNC(mace_device::vin2_r), FUNC(mace_device::vin2_w));
	map(0x00200000, 0x0027ffff).rw(FUNC(mace_device::vout_r), FUNC(mace_device::vout_w));
	map(0x00280000, 0x002fffff).rw(FUNC(mace_device::enet_r), FUNC(mace_device::enet_w));
	map(0x00300000, 0x0030ffff).rw(FUNC(mace_device::audio_r), FUNC(mace_device::audio_w));
	map(0x00310000, 0x0031ffff).rw(FUNC(mace_device::isa_r), FUNC(mace_device::isa_w));
	map(0x00320000, 0x0032ffff).rw(FUNC(mace_device::kbdms_r), FUNC(mace_device::kbdms_w));
	map(0x00330000, 0x0033ffff).rw(FUNC(mace_device::i2c_r), FUNC(mace_device::i2c_w));
	map(0x00340000, 0x0034ffff).rw(FUNC(mace_device::ust_msc_r), FUNC(mace_device::ust_msc_w));
	map(0x00380000, 0x0039ffff).rw(FUNC(mace_device::isa_ext_r), FUNC(mace_device::isa_ext_w));
	map(0x003a0000, 0x003a7fff).rw(FUNC(mace_device::rtc_r), FUNC(mace_device::rtc_w));
}

//**************************************************************************
//  REGISTER ACCESS
//**************************************************************************

uint64_t mace_device::pci_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_PCI, "%s: MACE: PCI: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f080000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::pci_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_PCI, "%s: MACE: PCI: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f080000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::vin1_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_VIN1, "%s: MACE: VIN1: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f100000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::vin1_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_VIN1, "%s: MACE: VIN1: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f100000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::vin2_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_VIN2, "%s: MACE: VIN2: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f180000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::vin2_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_VIN2, "%s: MACE: VIN2: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f180000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::vout_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_VOUT, "%s: MACE: VOUT: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f200000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::vout_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_VOUT, "%s: MACE: VOUT: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f200000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::enet_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_ENET, "%s: MACE: ENET: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f280000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::enet_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_ENET, "%s: MACE: ENET: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f280000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::audio_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_AUDIO, "%s: MACE: AUDIO: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f300000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::audio_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_AUDIO, "%s: MACE: AUDIO: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f300000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::isa_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	case 0x0000/8:
		ret = m_isa.m_ringbase_reset;
		LOGMASKED(LOG_READ_ISA, "%s: MACE: ISA: Ringbase Address Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0008/8:
		ret = m_isa.m_flash_nic_ctrl;
		LOGMASKED(LOG_HIFREQ, "%s: MACE: ISA: Flash/LED/DPRAM/NIC Control Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0010/8:
		ret = m_isa.m_int_status;
		LOGMASKED(LOG_READ_ISA, "%s: MACE: ISA: Interrupt Status Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0018/8:
		ret = m_isa.m_int_mask;
		LOGMASKED(LOG_READ_ISA, "%s: MACE: ISA: Interrupt Mask Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	default:
		LOGMASKED(LOG_READ_ISA, "%s: MACE: ISA: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f310000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::isa_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	case 0x0000/8:
		LOGMASKED(LOG_WRITE_ISA, "%s: MACE: ISA: Ringbase Address/Reset Write: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		m_isa.m_ringbase_reset = data;
		break;
	case 0x0008/8:
		LOGMASKED(LOG_HIFREQ, "%s: MACE: ISA: Flash/LED/DPRAM/NIC Control Write: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		LOGMASKED(LOG_HIFREQ, "           Enable Flash Writes: %d\n", BIT(data, 0));
		LOGMASKED(LOG_HIFREQ, "           NIC Deassert: %d\n", BIT(data, 2));
		LOGMASKED(LOG_HIFREQ, "           NIC Data: %d\n", BIT(data, 3));
		LOGMASKED(LOG_HIFREQ, "           Red LED: %d\n", BIT(data, 4));
		LOGMASKED(LOG_HIFREQ, "           Green LED: %d\n", BIT(data, 5));
		LOGMASKED(LOG_HIFREQ, "           DP-RAM Enable: %d\n", BIT(data, 6));
		m_isa.m_flash_nic_ctrl = data;
		break;
	case 0x0010/8:
		LOGMASKED(LOG_WRITE_ISA, "%s: MACE: ISA: Interrupt Status Write (Ignored): %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0018/8:
		LOGMASKED(LOG_WRITE_ISA, "%s: MACE: ISA: Interrupt Mask Write: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		m_isa.m_int_mask = data;
		break;
	default:
		LOGMASKED(LOG_WRITE_ISA, "%s: MACE: ISA: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f310000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::kbdms_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_KBDMS, "%s: MACE: KBDMS: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f320000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::kbdms_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_KBDMS, "%s: MACE: KBDMS: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f320000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::i2c_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_I2C, "%s: MACE: I2C: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f330000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::i2c_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_I2C, "%s: MACE: I2C: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f330000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

uint64_t mace_device::isa_ext_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	default:
		LOGMASKED(LOG_READ_ISA_EXT, "%s: MACE: ISA_EXT: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f380000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::isa_ext_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	default:
		LOGMASKED(LOG_WRITE_ISA_EXT, "%s: MACE: ISA_EXT: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f380000 + offset*8
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

//**************************************************************************
//  RTC
//**************************************************************************

uint64_t mace_device::rtc_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = m_rtc_read_callback(offset >> 5);

	LOGMASKED(LOG_READ_RTC, "%s: MACE: RTC Read: %08x (register %02x) = %08x%08x & %08x%08x\n", machine().describe_context()
		, 0x1f3a0000 + offset*8, offset >> 5, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);

	return ret;
}

void mace_device::rtc_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	LOGMASKED(LOG_WRITE_RTC, "%s: MACE: RTC Write: %08x (register %02x) = %08x%08x & %08x%08x\n", machine().describe_context(), 0x1f3a0000 + offset*8
			, offset >> 5, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);

	m_rtc_write_callback(offset >> 5, data & 0xff);
}

//**************************************************************************
//  TIMERS
//**************************************************************************

TIMER_CALLBACK_MEMBER(mace_device::ust_tick)
{
	m_ust_msc.m_ust++;
	m_ust_msc.m_ust_msc &= 0x00000000ffffffffULL;
	m_ust_msc.m_ust_msc |= (uint64_t)m_ust_msc.m_ust << 32;
	check_ust_msc_compare();
}

TIMER_CALLBACK_MEMBER(mace_device::msc_tick)
{
	m_ust_msc.m_msc++;
	m_ust_msc.m_ust_msc &= 0xffffffff00000000ULL;
	m_ust_msc.m_ust_msc |= m_ust_msc.m_msc;
	check_ust_msc_compare();
}

void mace_device::check_ust_msc_compare()
{
	if (m_ust_msc.m_ust_msc == m_ust_msc.m_compare1)
		m_isa.m_int_status |= ISA_INT_COMPARE1;
	if (m_ust_msc.m_ust_msc == m_ust_msc.m_compare2)
		m_isa.m_int_status |= ISA_INT_COMPARE2;
	if (m_ust_msc.m_ust_msc == m_ust_msc.m_compare3)
		m_isa.m_int_status |= ISA_INT_COMPARE3;
}

uint64_t mace_device::ust_msc_r(offs_t offset, uint64_t mem_mask)
{
	uint64_t ret = 0ULL;
	switch (offset)
	{
	case 0x0000/8:
		ret = ((uint64_t)m_ust_msc.m_ust << 32) | m_ust_msc.m_msc;
		LOGMASKED(LOG_HIFREQ, "%s: MACE: UST_MSC: MSC/UST Counter Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, m_ust_msc.m_ust, m_ust_msc.m_msc, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0008/8:
		ret = m_ust_msc.m_compare1;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Compare1 Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0010/8:
		ret = m_ust_msc.m_compare2;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Compare2 Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0018/8:
		ret = m_ust_msc.m_compare3;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Compare3 Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0020/8:
		ret = m_ust_msc.m_ain_msc_ust;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Audio In MSC/UST Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0028/8:
		ret = m_ust_msc.m_aout1_msc_ust;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Audio Out 1 MSC/UST Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0030/8:
		ret = m_ust_msc.m_aout2_msc_ust;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Audio Out 2 MSC/UST Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0038/8:
		ret = m_ust_msc.m_vin1_msc_ust;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Video In 1 MSC/UST Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0040/8:
		ret = m_ust_msc.m_vin2_msc_ust;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Video In 2 MSC/UST Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0048/8:
		ret = m_ust_msc.m_vout_msc_ust;
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Video Out MSC/UST Read: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(ret >> 32), (uint32_t)ret, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	default:
		LOGMASKED(LOG_READ_UST_MSC, "%s: MACE: UST_MSC: Unknown Read: [%08x] & %08x%08x\n", machine().describe_context()
			, 0x1f340000 + offset*8, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	}
	return ret;
}

void mace_device::ust_msc_w(offs_t offset, uint64_t data, uint64_t mem_mask)
{
	switch (offset)
	{
	case 0x0000/8:
		LOGMASKED(LOG_WRITE_UST_MSC, "%s: MACE: UST_MSC: MSC/UST Counter Write (Ignored): %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		break;
	case 0x0008/8:
		LOGMASKED(LOG_WRITE_UST_MSC, "%s: MACE: UST_MSC: Compare1 Write: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		m_ust_msc.m_compare1 = data;
		break;
	case 0x0010/8:
		LOGMASKED(LOG_WRITE_UST_MSC, "%s: MACE: UST_MSC: Compare2 Write: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		m_ust_msc.m_compare2 = data;
		break;
	case 0x0018/8:
		LOGMASKED(LOG_WRITE_UST_MSC, "%s: MACE: UST_MSC: Compare3 Write: %08x%08x & %08x%08x\n", machine().describe_context()
			, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		m_ust_msc.m_compare3 = data;
		break;
	default:
		LOGMASKED(LOG_WRITE_UST_MSC, "%s: MACE: UST_MSC: Unknown Write: %08x = %08x%08x & %08x%08x\n", machine().describe_context()
			, 0x1f340000 + offset*8, (uint32_t)(data >> 32), (uint32_t)data, (uint32_t)(mem_mask >> 32), (uint32_t)mem_mask);
		return;
	}
}

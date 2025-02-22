// license:BSD-3-Clause
// copyright-holders:hap
// thanks-to:Berger
/******************************************************************************

CXG Sphinx Dominator, chess computer.

Hardware notes:
- R65C02P4 @ 4MHz
- 32KB ROM, 8KB RAM battery-backed
- Sanyo LC7582, 2 LCD panels (each 4-digit)
- TTL, piezo, 8*8+8 LEDs, button sensors

Sphinx Galaxy is assumed to be on similar hardware.

The chess engine is by Frans Morsch, older versions (before 2.05) were buggy.
It's also used in the newer Mephisto Modena.

TODO:
- artwork
- buttons

******************************************************************************/

#include "emu.h"
#include "cpu/m6502/r65c02.h"
#include "machine/nvram.h"
#include "machine/sensorboard.h"
#include "sound/dac.h"
#include "sound/volt_reg.h"
#include "video/pwm.h"
#include "video/lc7582.h"
#include "speaker.h"

// internal artwork
//#include "cxg_dominator.lh" // clickable


namespace {

class dominator_state : public driver_device
{
public:
	dominator_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_lcd(*this, "lcd"),
		m_display(*this, "display"),
		m_board(*this, "board"),
		m_dac(*this, "dac"),
		m_inputs(*this, "IN.%u", 0),
		m_digits(*this, "digit%u", 0U),
		m_dp(*this, "dp%u", 0U)
	{ }

	// machine drivers
	void dominator(machine_config &config);

protected:
	virtual void machine_start() override;

private:
	// devices/pointers
	required_device<cpu_device> m_maincpu;
	required_device<lc7582_device> m_lcd;
	required_device<pwm_display_device> m_display;
	required_device<sensorboard_device> m_board;
	required_device<dac_bit_interface> m_dac;
	required_ioport_array<2> m_inputs;
	output_finder<8> m_digits;
	output_finder<2> m_dp;

	// address maps
	void main_map(address_map &map);

	// I/O handlers
	DECLARE_WRITE64_MEMBER(lcd_s_w);
	DECLARE_WRITE8_MEMBER(control_w);
	DECLARE_WRITE8_MEMBER(leds_w);
	DECLARE_READ8_MEMBER(input_r);
};

void dominator_state::machine_start()
{
	m_digits.resolve();
	m_dp.resolve();
}



/******************************************************************************
    I/O
******************************************************************************/

// LCD

WRITE64_MEMBER(dominator_state::lcd_s_w)
{
	u8 d[4];

	// 1st digit: S1-S9, unused middle vertical segments
	// 2nd digit: S10-S18, unused bottom-right diagonal segment
	// 3rd digit: S21-S27
	// 4th digit: S28-S34
	d[0] = bitswap<9>(data >> 0 & 0x1ff, 2,7,5,4,3,1,0,8,6) & 0x7f;
	d[1] = bitswap<9>(data >> 9 & 0x1ff, 7,6,4,2,0,8,5,3,1) & 0x7f;
	d[2] = bitswap<7>(data >> 20 & 0x7f, 3,5,1,0,2,6,4);
	d[3] = bitswap<7>(data >> 27 & 0x7f, 4,2,0,6,5,3,1);

	for (int i = 0; i < 4; i++)
		m_digits[offset * 4 + i] = d[i];

	// colon: S17 (part of 2nd digit)
	m_dp[offset] = BIT(data, 16);
}


// TTL

WRITE8_MEMBER(dominator_state::control_w)
{
	// d0: LC7582 DATA
	// d1: LC7582 CLK
	// d2: LC7582 CE
	m_lcd->data_w(BIT(data, 0));
	m_lcd->clk_w(BIT(data, 1));
	m_lcd->ce_w(BIT(data, 2));

	// d3: speaker out
	m_dac->write(BIT(data, 3));
}

WRITE8_MEMBER(dominator_state::leds_w)
{
	// led data
	m_display->matrix(1 << offset, data);
}

READ8_MEMBER(dominator_state::input_r)
{
	u8 data = 0;

	// read chessboard sensors
	if (offset < 8)
		data = m_board->read_rank(offset);

	// read other buttons
	else if (offset < 10)
		data = m_inputs[offset - 8]->read();

	return ~data;
}



/******************************************************************************
    Address Maps
******************************************************************************/

void dominator_state::main_map(address_map &map)
{
	map(0x0000, 0x1fff).ram(); //.share("nvram");
	map(0x4000, 0x400f).rw(FUNC(dominator_state::input_r), FUNC(dominator_state::leds_w));
	map(0x4010, 0x4010).w(FUNC(dominator_state::control_w));
	//map(0x7f00, 0x7fff).nopr(); // mid-opcode dummy read
	map(0x8000, 0xffff).rom();
}



/******************************************************************************
    Input Ports
******************************************************************************/

static INPUT_PORTS_START( dominator )
	PORT_START("IN.0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_3)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_6)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_8)

	PORT_START("IN.1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_U)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_CODE(KEYCODE_I)
INPUT_PORTS_END



/******************************************************************************
    Machine Drivers
******************************************************************************/

void dominator_state::dominator(machine_config &config)
{
	/* basic machine hardware */
	R65C02(config, m_maincpu, 4_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &dominator_state::main_map);

	const attotime irq_period = attotime::from_hz(4_MHz_XTAL / 0x2000); // from 4020
	m_maincpu->set_periodic_int(FUNC(dominator_state::nmi_line_pulse), irq_period);

	SENSORBOARD(config, m_board).set_type(sensorboard_device::BUTTONS);
	m_board->init_cb().set(m_board, FUNC(sensorboard_device::preset_chess));
	m_board->set_delay(attotime::from_msec(150));

	/* video hardware */
	LC7582(config, m_lcd, 0);
	m_lcd->write_segs().set(FUNC(dominator_state::lcd_s_w));

	PWM_DISPLAY(config, m_display).set_size(10, 8);
	//config.set_default_layout(layout_cxg_dominator);

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();
	DAC_1BIT(config, m_dac).add_route(ALL_OUTPUTS, "speaker", 0.25);
	VOLTAGE_REGULATOR(config, "vref").add_route(0, "dac", 1.0, DAC_VREF_POS_INPUT);
}



/******************************************************************************
    ROM Definitions
******************************************************************************/

ROM_START( sdtor )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("22p_2.05", 0x8000, 0x8000, CRC(9707119c) SHA1(d7cde835a37bd5d9ff349a871c890ea4cd9b2c26) )
ROM_END

} // anonymous namespace



/******************************************************************************
    Drivers
******************************************************************************/

/*    YEAR  NAME   PARENT  COMPAT  MACHINE    INPUT      CLASS            INIT        COMPANY, FULLNAME, FLAGS */
CONS( 1989, sdtor, 0,      0,      dominator, dominator, dominator_state, empty_init, "CXG", "Sphinx Dominator (v2.05)", MACHINE_SUPPORTS_SAVE | MACHINE_CLICKABLE_ARTWORK | MACHINE_NOT_WORKING )

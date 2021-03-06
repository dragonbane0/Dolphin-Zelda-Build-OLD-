// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cstddef>
#include <wx/bitmap.h>
#include <wx/checkbox.h>
#include <wx/dcmemory.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statbmp.h>
#include <wx/string.h>
#include <wx/textctrl.h>

#include <wx/msgdlg.h> //Dragonbane

#include <wx/app.h>
#include "Core/Core.h"
#include "Core/CoreParameter.h"
#include "Core/Boot/Boot.h"
#include "DolphinWX/Frame.h"
#include "DolphinWX/Globals.h"
#include "DolphinWX/Main.h"
#include "Core/HW/ProcessorInterface.h"
#include "Core/HW/Memmap.h"

#include "Common/CommonTypes.h"
#include "Core/Movie.h"
#include "Core/State.h"
#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"
#include "Core/HW/WiimoteEmu/Attachment/Nunchuk.h"
#include "Core/HW/WiimoteReal/WiimoteReal.h"
#include "DolphinWX/TASInputDlg.h"
#include "InputCommon/GCPadStatus.h"
#include "InputCommon/InputConfig.h"

#include <lua.hpp> //Dragonbane

//Dragonbane: Lua Stuff
static TASInputDlg *luaInstance;
static lua_State *luaState;

//Lua Functions
int ReadValue8(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	u32 address = lua_tointeger(L, 1);

	u8 result = Memory::Read_U8(address);

	lua_pushinteger(L, result); // return value
	return 1; // number of return values
}

int ReadValue16(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	u32 address = lua_tointeger(L, 1);

	u16 result = Memory::Read_U16(address);

	lua_pushinteger(L, result); // return value
	return 1; // number of return values
}

int ReadValue32(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	u32 address = lua_tointeger(L, 1);

	u32 result = Memory::Read_U32(address);

	lua_pushinteger(L, result); // return value
	return 1; // number of return values
}

int ReadValueFloat(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	u32 address = lua_tointeger(L, 1);

	float result = Memory::Read_F32(address);

	lua_pushnumber(L, result); // return value
	return 1; // number of return values
}
int ReadValueString(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 2)
		return 0;

	u32 address = lua_tointeger(L, 1);
	int count = lua_tointeger(L, 2);

	std::string result = Memory::Read_String(address, count);

	lua_pushstring(L, result.c_str()); // return value
	return 1; // number of return values
}

int GetPointerNormal(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	u32 address = lua_tointeger(L, 1);

	u32 pointer = Memory::Read_U32(address);

	if (pointer > 0x80000000)
	{
		pointer -= 0x80000000;
	}
	else
	{
		return 0;
	}

	lua_pushinteger(L, pointer); // return value
	return 1; // number of return values
}

int PressButton(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	const char* button = lua_tostring(L, 1);

	luaInstance->iPressButton(button);

	return 0; // number of return values
}

int ReleaseButton(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	const char* button = lua_tostring(L, 1);

	luaInstance->iReleaseButton(button);

	return 0; // number of return values
}

int SetMainStickX(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	int xPos = lua_tointeger(L, 1);

	luaInstance->iSetMainStickX(xPos);

	return 0;
}
int SetMainStickY(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	int yPos = lua_tointeger(L, 1);

	luaInstance->iSetMainStickY(yPos);

	return 0;
}

int SetCStickX(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	int xPos = lua_tointeger(L, 1);

	luaInstance->iSetCStickX(xPos);

	return 0;
}
int SetCStickY(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	int yPos = lua_tointeger(L, 1);

	luaInstance->iSetCStickY(yPos);

	return 0;
}

int GetFrameCount(lua_State *L)
{
	int argc = lua_gettop(L);

	lua_pushinteger(L, Movie::g_currentFrame); // return value
	return 1; // number of return values
}

int MsgBox(lua_State *L)
{
	int argc = lua_gettop(L);

	if (argc < 1)
		return 0;

	const char* text = lua_tostring(L, 1);

	std::string message = StringFromFormat("Lua Msg: %s", text);
	wxMessageBox(message);

	return 0; // number of return values
}

int AbortSwim(lua_State *L)
{
	int argc = lua_gettop(L);

	Movie::swimStarted = false;

	return 0; // number of return values
}

void HandleLuaErrors(lua_State *L, int status)
{
	if (status != 0)
	{
		std::string message = StringFromFormat("Lua Error: %s", lua_tostring(L, -1));
		wxMessageBox(message);

		lua_pop(L, 1); // remove error message
	}
}




TASInputDlg::TASInputDlg(wxWindow* parent, wxWindowID id, const wxString& title,
                         const wxPoint& position, const wxSize& size, long style)
: wxDialog(parent, id, title, position, size, style)
{

}

void TASInputDlg::CreateBaseLayout()
{
	for (unsigned int i = 0; i < 10; ++i)
		m_controls[i] = nullptr;
	for (unsigned int i = 0; i < 18; ++i) //Original: 17
		m_buttons[i] = nullptr;

	m_buttons[0] = &m_dpad_down;
	m_buttons[1] = &m_dpad_up;
	m_buttons[2] = &m_dpad_left;
	m_buttons[3] = &m_dpad_right;
	m_buttons[4] = &m_a;
	m_buttons[5] = &m_b;
	m_controls[0] = &m_main_stick.x_cont;
	m_controls[1] = &m_main_stick.y_cont;

	m_a = CreateButton("A");
	m_b = CreateButton("B");
	m_dpad_up = CreateButton("Up");
	m_dpad_right = CreateButton("Right");
	m_dpad_down = CreateButton("Down");
	m_dpad_left = CreateButton("Left");

	m_buttons_dpad = new wxGridSizer(3);
	m_buttons_dpad->AddSpacer(20);
	m_buttons_dpad->Add(m_dpad_up.checkbox, false);
	m_buttons_dpad->AddSpacer(20);
	m_buttons_dpad->Add(m_dpad_left.checkbox, false);
	m_buttons_dpad->AddSpacer(20);
	m_buttons_dpad->Add(m_dpad_right.checkbox, false);
	m_buttons_dpad->AddSpacer(20);
	m_buttons_dpad->Add(m_dpad_down.checkbox, false);
	m_buttons_dpad->AddSpacer(20);

	Bind(wxEVT_CLOSE_WINDOW, &TASInputDlg::OnCloseWindow, this);
	Bind(wxEVT_TEXT, &TASInputDlg::UpdateFromText, this);
}

const int TASInputDlg::m_gc_pad_buttons_bitmask[12] = {
	PAD_BUTTON_DOWN, PAD_BUTTON_UP, PAD_BUTTON_LEFT,PAD_BUTTON_RIGHT, PAD_BUTTON_A, PAD_BUTTON_B,
	PAD_BUTTON_X, PAD_BUTTON_Y, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_TRIGGER_R, PAD_BUTTON_START
};

const int TASInputDlg::m_wii_buttons_bitmask[13] = {
	WiimoteEmu::Wiimote::PAD_DOWN, WiimoteEmu::Wiimote::PAD_UP, WiimoteEmu::Wiimote::PAD_LEFT,
	WiimoteEmu::Wiimote::PAD_RIGHT, WiimoteEmu::Wiimote::BUTTON_A, WiimoteEmu::Wiimote::BUTTON_B,
	WiimoteEmu::Wiimote::BUTTON_ONE, WiimoteEmu::Wiimote::BUTTON_TWO, WiimoteEmu::Wiimote::BUTTON_PLUS,
	WiimoteEmu::Wiimote::BUTTON_MINUS, WiimoteEmu::Wiimote::BUTTON_HOME,
};

void TASInputDlg::CreateWiiLayout(int num)
{
	if (m_has_layout)
		return;

	CreateBaseLayout();

	m_buttons[6] = &m_one;
	m_buttons[7] = &m_two;
	m_buttons[8] = &m_plus;
	m_buttons[9] = &m_minus;
	m_buttons[10] = &m_home;

	m_controls[4] = &m_x_cont;
	m_controls[5] = &m_y_cont;
	m_controls[6] = &m_z_cont;

	m_main_stick = CreateStick(ID_MAIN_STICK, 1024, 768, 512, 384, true, false);
	m_main_stick_szr = CreateStickLayout(&m_main_stick, _("IR"));

	m_x_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 1023, 512);
	m_y_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 1023, 512);
	m_z_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 1023, 616);
	wxStaticBoxSizer* const axisBox = CreateAccelLayout(&m_x_cont, &m_y_cont, &m_z_cont, _("Orientation"));

	wxStaticBoxSizer* const m_buttons_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Buttons"));
	wxGridSizer* const m_buttons_grid = new wxGridSizer(4);

	m_plus = CreateButton("+");
	m_minus = CreateButton("-");
	m_one = CreateButton("1");
	m_two = CreateButton("2");
	m_home = CreateButton("Home");

	m_main_szr = new wxBoxSizer(wxVERTICAL);
	m_wiimote_szr = new wxBoxSizer(wxHORIZONTAL);
	m_ext_szr = new wxBoxSizer(wxHORIZONTAL);

	if (Core::IsRunning())
	{
		m_ext = ((WiimoteEmu::Wiimote*)Wiimote::GetConfig()->controllers[num])->CurrentExtension();
	}
	else
	{
		IniFile ini;
		ini.Load(File::GetUserPath(D_CONFIG_IDX) + "WiimoteNew.ini");
		std::string extension;
		ini.GetIfExists("Wiimote" + std::to_string(num+1), "Extension", &extension);

		if (extension == "Nunchuk")
			m_ext = 1;
		if (extension == "Classic Controller")
			m_ext = 2;
	}

	m_buttons[11] = &m_c;
	m_buttons[12] = &m_z;
	m_controls[2] = &m_c_stick.x_cont;
	m_controls[3] = &m_c_stick.y_cont;
	m_controls[7] = &m_nx_cont;
	m_controls[8] = &m_ny_cont;
	m_controls[9] = &m_nz_cont;

	m_c_stick = CreateStick(ID_C_STICK, 255, 255, 128, 128, false, true);
	m_c_stick_szr = CreateStickLayout(&m_c_stick, _("Nunchuk stick"));

	m_nx_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 1023, 512);
	m_ny_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 1023, 512);
	m_nz_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 1023, 512);
	wxStaticBoxSizer* const nunchukaxisBox = CreateAccelLayout(&m_nx_cont, &m_ny_cont, &m_nz_cont, _("Nunchuk orientation"));

	m_c = CreateButton("C");
	m_z = CreateButton("Z");
	m_ext_szr->Add(m_c_stick_szr, 0, wxLEFT | wxBOTTOM | wxRIGHT, 5);
	m_ext_szr->Add(nunchukaxisBox);

	for (Control* const control : m_controls)
	{
		if (control != nullptr)
			control->slider->Bind(wxEVT_RIGHT_UP, &TASInputDlg::OnRightClickSlider, this);
	}

	for (unsigned int i = 4; i < 14; ++i)
		if (m_buttons[i] != nullptr)
			m_buttons_grid->Add(m_buttons[i]->checkbox, false);
	m_buttons_grid->AddSpacer(5);

	m_buttons_box->Add(m_buttons_grid);
	m_buttons_box->Add(m_buttons_dpad);

	m_wiimote_szr->Add(m_main_stick_szr, 0, wxALL, 5);
	m_wiimote_szr->Add(axisBox, 0, wxTOP | wxRIGHT, 5);
	m_wiimote_szr->Add(m_buttons_box, 0, wxTOP | wxRIGHT, 5);
	m_main_szr->Add(m_wiimote_szr);
	m_main_szr->Add(m_ext_szr);
	if (m_ext == 1)
		m_main_szr->Show(m_ext_szr);
	else
		m_main_szr->Hide(m_ext_szr);
	SetSizerAndFit(m_main_szr, true);

	ResetValues();
	m_has_layout = true;
}

void TASInputDlg::CreateGCLayout()
{
	if (m_has_layout)
		return;

	CreateBaseLayout();

	m_buttons[6] = &m_x;
	m_buttons[7] = &m_y;
	m_buttons[8] = &m_z;
	m_buttons[9] = &m_l;
	m_buttons[10] = &m_r;
	m_buttons[11] = &m_start;

	//Dragonbane: Special
	m_buttons[14] = &m_reset;
	m_buttons[15] = &m_quickspin;
	m_buttons[16] = &m_rollassist;
	m_buttons[17] = &m_skipDialog;


	m_controls[2] = &m_c_stick.x_cont;
	m_controls[3] = &m_c_stick.y_cont;
	m_controls[4] = &m_l_cont;
	m_controls[5] = &m_r_cont;

	wxBoxSizer* const top_box = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* const bottom_box = new wxBoxSizer(wxHORIZONTAL);
	m_main_stick = CreateStick(ID_MAIN_STICK, 255, 255, 128, 128, false, true);
	wxStaticBoxSizer* const main_box = CreateStickLayout(&m_main_stick, _("Main Stick"));

	m_c_stick = CreateStick(ID_C_STICK, 255, 255, 128, 128, false, true);
	wxStaticBoxSizer* const c_box = CreateStickLayout(&m_c_stick, _("C Stick"));

	wxStaticBoxSizer* const shoulder_box = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Shoulder Buttons"));
	m_l_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 255, 0);
	m_r_cont = CreateControl(wxSL_VERTICAL, -1, 100, false, 255, 0);
	shoulder_box->Add(m_l_cont.slider, 0, wxALIGN_CENTER_VERTICAL);
	shoulder_box->Add(m_l_cont.text, 0, wxALIGN_CENTER_VERTICAL);
	shoulder_box->Add(m_r_cont.slider, 0, wxALIGN_CENTER_VERTICAL);
	shoulder_box->Add(m_r_cont.text, 0, wxALIGN_CENTER_VERTICAL);

	for (Control* const control : m_controls)
	{
		if (control != nullptr)
			control->slider->Bind(wxEVT_RIGHT_UP, &TASInputDlg::OnRightClickSlider, this);
	}

	wxStaticBoxSizer* const m_buttons_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Buttons"));
	wxGridSizer* const m_buttons_grid = new wxGridSizer(4);

	m_x = CreateButton("X");
	m_y = CreateButton("Y");
	m_l = CreateButton("L");
	m_r = CreateButton("R");
	m_z = CreateButton("Z");
	m_start = CreateButton("Start");

	for (unsigned int i = 4; i < 14; ++i) //17
		if (m_buttons[i] != nullptr)
			m_buttons_grid->Add(m_buttons[i]->checkbox, false);
	m_buttons_grid->AddSpacer(5);

	m_buttons_box->Add(m_buttons_grid);
	m_buttons_box->Add(m_buttons_dpad);



	//Dragonbane: Special Box
	wxStaticBoxSizer* const m_buttons_extra = new wxStaticBoxSizer(wxVERTICAL, this, _("Extras"));
	wxGridSizer* const m_buttons_grid_extra = new wxGridSizer(1);

	m_reset = CreateButton("Reset");
	m_quickspin = CreateButton("Quick Spin");
	m_rollassist = CreateButton("Auto Roll");
	m_skipDialog = CreateButton("Auto Dialog");

	m_rollassist.checkbox->Bind(wxEVT_CHECKBOX, &TASInputDlg::UpdateFromButtons, this);

	for (unsigned int i = 14; i < 18; ++i)
		if (m_buttons[i] != nullptr)
			m_buttons_grid_extra->Add(m_buttons[i]->checkbox, false);
	m_buttons_grid_extra->AddSpacer(5);

	m_buttons_extra->Add(m_buttons_grid_extra);


	wxBoxSizer* const main_szr = new wxBoxSizer(wxVERTICAL);

	top_box->Add(main_box, 0, wxALL, 5);
	top_box->Add(c_box, 0, wxTOP | wxRIGHT, 5);
	bottom_box->Add(shoulder_box, 0, wxLEFT | wxRIGHT, 5);
	bottom_box->Add(m_buttons_box, 0, wxBOTTOM, 5);

	//Dragonban
	bottom_box->Add(m_buttons_extra, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

	main_szr->Add(top_box);
	main_szr->Add(bottom_box);
	SetSizerAndFit(main_szr);

	ResetValues();
	m_has_layout = true;
}


TASInputDlg::Control TASInputDlg::CreateControl(long style, int width, int height, bool reverse, u32 range, u32 default_value)
{
	Control tempCont;
	tempCont.range = range;
	tempCont.default_value = default_value;
	tempCont.slider = new wxSlider(this, m_eleID++, default_value, 0, range, wxDefaultPosition, wxDefaultSize, style);
	tempCont.slider->SetMinSize(wxSize(width, height));
	tempCont.slider->Bind(wxEVT_SLIDER, &TASInputDlg::UpdateFromSliders, this);
	tempCont.text = new wxTextCtrl(this, m_eleID++, std::to_string(default_value), wxDefaultPosition, wxSize(40, 20));
	tempCont.text->SetMaxLength(range > 999 ? 4 : 3);
	tempCont.text_id = m_eleID - 1;
	tempCont.text->Bind(wxEVT_TEXT, &TASInputDlg::UpdateFromText, this);
	tempCont.slider_id = m_eleID - 2;
	tempCont.reverse = reverse;
	return tempCont;
}

TASInputDlg::Stick TASInputDlg::CreateStick(int id_stick, int xRange, int yRange, u32 defaultX, u32 defaultY, bool reverseX, bool reverseY)
{
	Stick tempStick;
	tempStick.bitmap = new wxStaticBitmap(this, id_stick, CreateStickBitmap(128, 128));
	tempStick.bitmap->Bind(wxEVT_MOTION, &TASInputDlg::OnMouseDownL, this);
	tempStick.bitmap->Bind(wxEVT_LEFT_DOWN, &TASInputDlg::OnMouseDownL, this);
	tempStick.bitmap->Bind(wxEVT_RIGHT_UP, &TASInputDlg::OnMouseUpR, this);
	tempStick.x_cont = CreateControl(wxSL_HORIZONTAL | (reverseX ? wxSL_INVERSE : 0), 120, -1, reverseX, xRange, defaultX);
	tempStick.y_cont = CreateControl(wxSL_VERTICAL | (reverseY ?  wxSL_INVERSE : 0), -1, 120, reverseY, yRange, defaultY);
	return tempStick;
}

wxStaticBoxSizer* TASInputDlg::CreateStickLayout(Stick* tempStick, const wxString& title)
{
	wxStaticBoxSizer* const temp_box = new wxStaticBoxSizer(wxHORIZONTAL, this, title);
	wxBoxSizer* const temp_xslider_box = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* const temp_yslider_box = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* const temp_stick_box = new wxBoxSizer(wxVERTICAL);

	temp_xslider_box->Add(tempStick->x_cont.slider, 0, wxALIGN_TOP);
	temp_xslider_box->Add(tempStick->x_cont.text, 0, wxALIGN_TOP);
	temp_stick_box->Add(temp_xslider_box);
	temp_stick_box->Add(tempStick->bitmap, 0, wxALL | wxALIGN_CENTER, 3);
	temp_box->Add(temp_stick_box);
	temp_yslider_box->Add(tempStick->y_cont.slider, 0, wxALIGN_CENTER_VERTICAL);
	temp_yslider_box->Add(tempStick->y_cont.text, 0, wxALIGN_CENTER_VERTICAL);
	temp_box->Add(temp_yslider_box);
	return temp_box;
}

wxStaticBoxSizer* TASInputDlg::CreateAccelLayout(Control* x, Control* y, Control* z, const wxString& title)
{
	wxStaticBoxSizer* const temp_box = new wxStaticBoxSizer(wxHORIZONTAL, this, title);
	wxStaticBoxSizer* const xBox = new wxStaticBoxSizer(wxVERTICAL, this, _("X"));
	wxStaticBoxSizer* const yBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Y"));
	wxStaticBoxSizer* const zBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Z"));

	xBox->Add(x->slider, 0, wxALIGN_CENTER_VERTICAL);
	xBox->Add(x->text, 0, wxALIGN_CENTER_VERTICAL);
	yBox->Add(y->slider, 0, wxALIGN_CENTER_VERTICAL);
	yBox->Add(y->text, 0, wxALIGN_CENTER_VERTICAL);
	zBox->Add(z->slider, 0, wxALIGN_CENTER_VERTICAL);
	zBox->Add(z->text, 0, wxALIGN_CENTER_VERTICAL);
	temp_box->Add(xBox, 0, wxLEFT | wxBOTTOM | wxRIGHT, 5);
	temp_box->Add(yBox, 0, wxRIGHT, 5);
	temp_box->Add(zBox, 0, wxRIGHT, 5);
	return temp_box;
}

TASInputDlg::Button TASInputDlg::CreateButton(const std::string& name)
{
	Button temp;
	wxCheckBox* checkbox = new wxCheckBox(this, m_eleID++, name);
	checkbox->Bind(wxEVT_RIGHT_DOWN, &TASInputDlg::SetTurbo, this);
	checkbox->Bind(wxEVT_LEFT_DOWN, &TASInputDlg::SetTurbo, this);
	temp.checkbox = checkbox;
	temp.id = m_eleID - 1;
	return temp;
}

void TASInputDlg::ResetValues()
{
	for (Button* const button : m_buttons)
	{
		if (button != nullptr)
			button->checkbox->SetValue(false);
	}

	for (Control* const control : m_controls)
	{
		if (control != nullptr)
		{
			control->value = control->default_value;
			control->slider->SetValue(control->default_value);
			control->text->SetValue(std::to_string(control->default_value));
		}
	}
}

void TASInputDlg::SetStickValue(bool* ActivatedByKeyboard, int* AmountPressed, wxTextCtrl* Textbox, int CurrentValue, int center)
{
	if (CurrentValue != center)
	{
		*AmountPressed = CurrentValue;
		*ActivatedByKeyboard = true;
	}
	else if (*ActivatedByKeyboard)
	{
		*AmountPressed = center;
		*ActivatedByKeyboard = false;
	}
	else
	{
		return;
	}
	
	Textbox->ChangeValue(std::to_string(*AmountPressed));
	wxCommandEvent* evt = new wxCommandEvent(wxEVT_TEXT, Textbox->GetId());
	evt->SetEventObject(Textbox);
	wxQueueEvent(this, evt);
}

void TASInputDlg::SetSliderValue(Control* control, int CurrentValue, int default_value)
{
	if (CurrentValue != default_value)
	{
		control->value = CurrentValue;
		control->set_by_keyboard = true;
		control->text->ChangeValue(std::to_string(CurrentValue));
	}
	else if (control->set_by_keyboard)
	{
		control->value = default_value;
		control->set_by_keyboard = false;
		control->text->ChangeValue(std::to_string(default_value));
	}
	else
	{
		return;
	}
	
	wxCommandEvent* evt = new wxCommandEvent(wxEVT_TEXT, control->text_id);
	evt->SetEventObject(control->text);
	wxQueueEvent(this, evt);
}

void TASInputDlg::SetButtonValue(Button* button, bool CurrentState)
{
	if (CurrentState)
	{
		button->set_by_keyboard = true;
		button->checkbox->SetValue(CurrentState);
	}
	else if (button->set_by_keyboard)
	{
		button->set_by_keyboard = false;
		button->checkbox->SetValue(CurrentState);
	}
}

void TASInputDlg::SetWiiButtons(u16* butt)
{
	for (unsigned int i = 0; i < 11; ++i)
	{
		if (m_buttons[i] != nullptr)
			*butt |= (m_buttons[i]->checkbox->IsChecked()) ? m_wii_buttons_bitmask[i] : 0;
	}
	ButtonTurbo();
}

void TASInputDlg::GetKeyBoardInput(GCPadStatus* PadStatus)
{
	SetStickValue(&m_main_stick.x_cont.set_by_keyboard, &m_main_stick.x_cont.value, m_main_stick.x_cont.text, PadStatus->stickX);
	SetStickValue(&m_main_stick.y_cont.set_by_keyboard, &m_main_stick.y_cont.value, m_main_stick.y_cont.text, PadStatus->stickY);

	SetStickValue(&m_c_stick.x_cont.set_by_keyboard, &m_c_stick.x_cont.value, m_c_stick.x_cont.text, PadStatus->substickX);
	SetStickValue(&m_c_stick.y_cont.set_by_keyboard, &m_c_stick.y_cont.value, m_c_stick.y_cont.text, PadStatus->substickY);
	SetSliderValue(&m_l_cont, PadStatus->triggerLeft, 0);
	SetSliderValue(&m_r_cont, PadStatus->triggerRight, 0);

	for (unsigned int i = 0; i < 14; ++i)
	{
		if (m_buttons[i] != nullptr)
			SetButtonValue(m_buttons[i], ((PadStatus->button & m_gc_pad_buttons_bitmask[i]) != 0));
	}
	SetButtonValue(&m_l, ((PadStatus->triggerLeft) == 255) || ((PadStatus->button & PAD_TRIGGER_L) != 0));
	SetButtonValue(&m_r, ((PadStatus->triggerRight) == 255) || ((PadStatus->button & PAD_TRIGGER_R) != 0));
}

void TASInputDlg::GetKeyBoardInput(u8* data, WiimoteEmu::ReportFeatures rptf, int ext, const wiimote_key key)
{
	u8* const coreData = rptf.core ? (data + rptf.core) : nullptr;
	u8* const accelData = rptf.accel ? (data + rptf.accel) : nullptr;
	//u8* const irData = rptf.ir ? (data + rptf.ir) : nullptr;
	u8* const extData = rptf.ext ? (data + rptf.ext) : nullptr;

	if (coreData)
	{
		for (unsigned int i = 0; i < 11; ++i)
		{
			if (m_buttons[i] != nullptr)
				SetButtonValue(m_buttons[i], (((wm_buttons*)coreData)->hex & m_wii_buttons_bitmask[i]) != 0);
		}
	}
	if (accelData)
	{
		wm_accel* dt = (wm_accel*)accelData;

		SetSliderValue(&m_x_cont, dt->x << 2 | ((wm_buttons*)coreData)->acc_x_lsb, m_x_cont.default_value);
		SetSliderValue(&m_y_cont, dt->y << 2 | ((wm_buttons*)coreData)->acc_y_lsb << 1, m_y_cont.default_value);
		SetSliderValue(&m_z_cont, dt->z << 2 | ((wm_buttons*)coreData)->acc_z_lsb << 1, m_z_cont.default_value);
	}

	// I don't think this can be made to work in a sane manner.
	//if (irData)
	//{
	//	u16 x = 1023 - (irData[0] | ((irData[2] >> 4 & 0x3) << 8));
	//	u16 y = irData[1] | ((irData[2] >> 6 & 0x3) << 8);

	//	SetStickValue(&m_main_stick.x_cont.set_by_keyboard, &m_main_stick.x_cont.value, m_main_stick.x_cont.text, x, 561);
	//	SetStickValue(&m_main_stick.y_cont.set_by_keyboard, &m_main_stick.y_cont.value, m_main_stick.y_cont.text, y, 486);
	//}

	if (extData && ext == 1)
	{
		wm_nc& nunchuk = *(wm_nc*)extData;
		WiimoteDecrypt(&key, (u8*)&nunchuk, 0, sizeof(wm_nc));
		nunchuk.bt.hex = nunchuk.bt.hex ^ 0x3;
		SetButtonValue(m_buttons[11], nunchuk.bt.c != 0);
		SetButtonValue(m_buttons[12], nunchuk.bt.z != 0);
	}
}

void TASInputDlg::GetValues(u8* data, WiimoteEmu::ReportFeatures rptf, int ext, const wiimote_key key)
{
	if (!IsShown() || !m_has_layout)
		return;

	GetKeyBoardInput(data, rptf, ext, key);

	u8* const coreData = rptf.core ? (data + rptf.core) : nullptr;
	u8* const accelData = rptf.accel ? (data + rptf.accel) : nullptr;
	u8* const irData = rptf.ir ? (data + rptf.ir) : nullptr;
	u8* const extData = rptf.ext ? (data + rptf.ext) : nullptr;

	if (coreData)
		SetWiiButtons(&((wm_buttons*)coreData)->hex);

	if (accelData)
	{
		wm_accel& dt = *(wm_accel*)accelData;
		wm_buttons& but = *(wm_buttons*)coreData;
		dt.x = m_x_cont.value >> 2;
		dt.y = m_y_cont.value >> 2;
		dt.z = m_z_cont.value >> 2;
		but.acc_x_lsb = m_x_cont.value & 0x3;
		but.acc_y_lsb = m_y_cont.value >> 1 & 0x1;
		but.acc_z_lsb = m_z_cont.value >> 1 & 0x1;
	}
	if (irData)
	{
		u16 x[4];
		u16 y;

		x[0] = m_main_stick.x_cont.value;
		y = m_main_stick.y_cont.value;
		x[1] = x[0] + 100;
		x[2] = x[0] - 10;
		x[3] = x[1] + 10;

		u8 mode;
		// Mode 5 not supported in core anyway.
		if (rptf.ext)
			mode = (rptf.ext - rptf.ir) == 10 ? 1 : 3;
		else
			mode = (rptf.size - rptf.ir) == 10 ? 1 : 3;

		if (mode == 1)
		{
			memset(irData, 0xFF, sizeof(wm_ir_basic) * 2);
			wm_ir_basic* ir_data = (wm_ir_basic*)irData;
			for (unsigned int i = 0; i < 2; ++i)
			{
				if (x[i*2] < 1024 && y < 768)
				{
					ir_data[i].x1 = static_cast<u8>(x[i*2]);
					ir_data[i].x1hi = x[i*2] >> 8;

					ir_data[i].y1 = static_cast<u8>(y);
					ir_data[i].y1hi = y >> 8;
				}
				if (x[i*2+1] < 1024 && y < 768)
				{
					ir_data[i].x2 = static_cast<u8>(x[i*2+1]);
					ir_data[i].x2hi = x[i*2+1] >> 8;

					ir_data[i].y2 = static_cast<u8>(y);
					ir_data[i].y2hi = y >> 8;
				}
			}
		}
		else
		{
			memset(data, 0xFF, sizeof(wm_ir_extended) * 4);
			wm_ir_extended* const ir_data = (wm_ir_extended*)irData;
			for (unsigned int i = 0; i < 4; ++i)
			{
				if (x[i] < 1024 && y < 768)
				{
					ir_data[i].x = static_cast<u8>(x[i]);
					ir_data[i].xhi = x[i] >> 8;

					ir_data[i].y = static_cast<u8>(y);
					ir_data[i].yhi = y >> 8;

					ir_data[i].size = 10;
				}
			}
		}
	}
	if (ext != m_ext)
	{
		m_ext = ext;
		if (ext == 0)
		{
			m_main_szr->Hide(m_ext_szr);
		}
		else
		{
			m_main_szr->Show(m_ext_szr);
		}
		SetSizerAndFit(m_main_szr);
	}
	else if (extData && ext == 1)
	{
		wm_nc& nunchuk = *(wm_nc*)extData;

		nunchuk.jx = m_c_stick.x_cont.value;
		nunchuk.jy = m_c_stick.y_cont.value;

		nunchuk.ax    = m_nx_cont.value >> 2;
		nunchuk.bt.acc_x_lsb = m_nx_cont.value & 0x3;
		nunchuk.ay    = m_ny_cont.value >> 2;
		nunchuk.bt.acc_y_lsb = m_ny_cont.value & 0x3;
		nunchuk.az    = m_nz_cont.value >> 2;
		nunchuk.bt.acc_z_lsb = m_nz_cont.value & 0x3;

		nunchuk.bt.hex |= (m_buttons[11]->checkbox->IsChecked()) ? WiimoteEmu::Nunchuk::BUTTON_C : 0;
		nunchuk.bt.hex |= (m_buttons[12]->checkbox->IsChecked()) ? WiimoteEmu::Nunchuk::BUTTON_Z : 0;
		nunchuk.bt.hex = nunchuk.bt.hex ^ 0x3;
		WiimoteEncrypt(&key, (u8*)&nunchuk, 0, sizeof(wm_nc));
	}
	//else if (extData && ext == 2)
	//{
		// TODO
		//wm_classic_extension& cc = *(wm_classic_extension*)extData;
		//WiimoteDecrypt(&key, (u8*)&cc, 0, sizeof(wm_classic_extension));

		//WiimoteEncrypt(&key, (u8*)&cc, 0, sizeof(wm_classic_extension));
	//}
}

void TASInputDlg::GetValues(GCPadStatus* PadStatus)
{
	if (!IsShown())
		return;

	//TODO:: Make this instant not when polled.
	GetKeyBoardInput(PadStatus);

	//Dragonbane: Execute custom scripts
	ExecuteScripts();

	PadStatus->stickX = m_main_stick.x_cont.value;
	PadStatus->stickY = m_main_stick.y_cont.value;
	PadStatus->substickX = m_c_stick.x_cont.value;
	PadStatus->substickY = m_c_stick.y_cont.value;
	PadStatus->triggerLeft = m_l.checkbox->GetValue() ? 255 : m_l_cont.value; //m_l_cont.slider->GetValue();
	PadStatus->triggerRight = m_r.checkbox->GetValue() ? 255 : m_r_cont.value; //m_r_cont.slider->GetValue();

	for (unsigned int i = 0; i < 14; ++i)
	{
		if (m_buttons[i] != nullptr)
		{
			if (m_buttons[i]->checkbox->IsChecked())
				PadStatus->button |= m_gc_pad_buttons_bitmask[i];
			else
				PadStatus->button &= ~m_gc_pad_buttons_bitmask[i];
		}
	}

	if (m_a.checkbox->IsChecked())
		PadStatus->analogA = 0xFF;
	else
		PadStatus->analogA = 0x00;

	if (m_b.checkbox->IsChecked())
		PadStatus->analogB = 0xFF;
	else
		PadStatus->analogB = 0x00;

	ButtonTurbo();
}

void TASInputDlg::UpdateFromSliders(wxCommandEvent& event)
{
	wxTextCtrl* text = nullptr;

	for (Control* const control : m_controls)
	{
		if (control != nullptr && event.GetId() == control->slider_id)
			text = control->text;
	}

	int value = ((wxSlider*) event.GetEventObject())->GetValue();
	if (text)
		text->SetValue(std::to_string(value));
}

void TASInputDlg::UpdateFromText(wxCommandEvent& event)
{
	unsigned long value;

	if (!((wxTextCtrl*) event.GetEventObject())->GetValue().ToULong(&value))
		return;

	for (Control* const control : m_controls)
	{
		if (control != nullptr && event.GetId() == control->text_id)
		{
			int v = (value > control->range) ? control->range : value;
			control->slider->SetValue(v);
			control->value = v;
		}
	}

	if (m_controls[2] != nullptr)
	{
		int x = m_c_stick.x_cont.value;
		int y = m_c_stick.y_cont.value;

		if (m_c_stick.x_cont.reverse)
			x = m_c_stick.x_cont.range - m_c_stick.x_cont.value + 1;
		if (m_c_stick.y_cont.reverse)
			y = m_c_stick.y_cont.range - m_c_stick.y_cont.value + 1;

		m_c_stick.bitmap->SetBitmap(CreateStickBitmap(x, y));
	}

	int x = (u8)(std::floor(((double)m_main_stick.x_cont.value / (double)m_main_stick.x_cont.range * 255.0) + .5));
	int y = (u8)(std::floor(((double)m_main_stick.y_cont.value / (double)m_main_stick.y_cont.range * 255.0) + .5));
	if (m_main_stick.x_cont.reverse)
		x = 256 - (u8)x;
	if (m_main_stick.y_cont.reverse)
		y = 256 - (u8)y;
	m_main_stick.bitmap->SetBitmap(CreateStickBitmap(x, y));
}

void TASInputDlg::OnCloseWindow(wxCloseEvent& event)
{
	if (event.CanVeto())
	{
		event.Skip(false);
		Show(false);
		ResetValues();
	}
}

bool TASInputDlg::TASHasFocus()
{
	if (!m_has_layout)
		return false;
	//allows numbers to be used as hotkeys
	for (Control* const control : m_controls)
	{
		if (control != nullptr && wxWindow::FindFocus() == control->text)
			return false;
	}

	if (wxWindow::FindFocus() == this)
		return true;
	else if (wxWindow::FindFocus() != nullptr && wxWindow::FindFocus()->GetParent() == this)
		return true;
	else
		return false;
}

void TASInputDlg::OnMouseUpR(wxMouseEvent& event)
{
	Stick* stick = nullptr;
	if (event.GetId() == ID_MAIN_STICK)
		stick = &m_main_stick;
	else if (event.GetId() == ID_C_STICK)
		stick = &m_c_stick;

	if (stick == nullptr)
		return;

	stick->x_cont.value = stick->x_cont.default_value;
	stick->y_cont.value = stick->y_cont.default_value;
	stick->bitmap->SetBitmap(CreateStickBitmap(128,128));
	stick->x_cont.text->SetValue(std::to_string(stick->x_cont.default_value));
	stick->y_cont.text->SetValue(std::to_string(stick->y_cont.default_value));
	stick->x_cont.slider->SetValue(stick->x_cont.default_value);
	stick->y_cont.slider->SetValue(stick->y_cont.default_value);

	event.Skip();
}

void TASInputDlg::OnRightClickSlider(wxMouseEvent& event)
{
	for (Control* const control : m_controls)
	{
		if (control != nullptr && event.GetId() == control->slider_id)
		{
			control->value = control->default_value;
			control->slider->SetValue(control->default_value);
			control->text->SetValue(std::to_string(control->default_value));
		}
	}
}

void TASInputDlg::OnMouseDownL(wxMouseEvent& event)
{
	if (!event.LeftIsDown())
		return;

	Stick* stick;
	if (event.GetId() == ID_MAIN_STICK)
		stick = &m_main_stick;
	else if (event.GetId() == ID_C_STICK)
		stick = &m_c_stick;
	else
		return;

	wxPoint ptM(event.GetPosition());
	stick->x_cont.value = ptM.x * stick->x_cont.range / 127;
	stick->y_cont.value = ptM.y * stick->y_cont.range / 127;

	if ((unsigned)stick->y_cont.value > stick->y_cont.range)
		stick->y_cont.value = stick->y_cont.range;
	if ((unsigned)stick->x_cont.value > stick->x_cont.range)
		stick->x_cont.value = stick->x_cont.range;

	if (stick->y_cont.reverse)
		stick->y_cont.value = stick->y_cont.range - (u16)stick->y_cont.value;
	if (stick->x_cont.reverse)
		stick->x_cont.value = stick->x_cont.range - (u16)stick->x_cont.value;

	stick->x_cont.value = (unsigned int)stick->x_cont.value > stick->x_cont.range ? stick->x_cont.range : stick->x_cont.value;
	stick->y_cont.value = (unsigned int)stick->y_cont.value > stick->y_cont.range ? stick->y_cont.range : stick->y_cont.value;

	stick->bitmap->SetBitmap(CreateStickBitmap(ptM.x*2, ptM.y*2));

	stick->x_cont.text->SetValue(std::to_string(stick->x_cont.value));
	stick->y_cont.text->SetValue(std::to_string(stick->y_cont.value));

	stick->x_cont.slider->SetValue(stick->x_cont.value);
	stick->y_cont.slider->SetValue(stick->y_cont.value);
	event.Skip();
}

void TASInputDlg::SetTurbo(wxMouseEvent& event)
{
	Button* button = nullptr;

	for (Button* const btn : m_buttons)
	{
		if (btn != nullptr && event.GetId() == btn->id)
			button = btn;
	}

	if (event.LeftDown())
	{
		if (button)
			button->turbo_on = false;

		event.Skip();
		return;
	}

	if (button)
	{
		button->checkbox->SetValue(true);
		button->turbo_on = !button->turbo_on;
	}

	event.Skip();
}

void TASInputDlg::ButtonTurbo()
{
	static u64 frame = Movie::g_currentFrame;

	if (frame != Movie::g_currentFrame)
	{
		frame = Movie::g_currentFrame;
		for (Button* const button : m_buttons)
		{
			if (button != nullptr && button->turbo_on)
				button->checkbox->SetValue(!button->checkbox->GetValue());
		}
	}
}

wxBitmap TASInputDlg::CreateStickBitmap(int x, int y)
{
	x = x / 2;
	y = y / 2;

	wxMemoryDC memDC;
	wxBitmap bitmap(129, 129);
	memDC.SelectObject(bitmap);
	memDC.SetBackground(*wxLIGHT_GREY_BRUSH);
	memDC.Clear();
	memDC.SetBrush(*wxWHITE_BRUSH);
	memDC.DrawCircle(65, 65, 64);
	memDC.SetBrush(*wxRED_BRUSH);
	memDC.DrawLine(64, 64, x, y);
	memDC.DrawLine(63, 64, x - 1, y);
	memDC.DrawLine(65, 64, x + 1 , y);
	memDC.DrawLine(64, 63, x, y - 1);
	memDC.DrawLine(64, 65, x, y + 1);
	memDC.SetPen(*wxBLACK_PEN);
	memDC.CrossHair(64, 64);
	memDC.SetBrush(*wxBLUE_BRUSH);
	memDC.DrawCircle(x, y, 5);
	memDC.SelectObject(wxNullBitmap);
	return bitmap;
}

//Dragonbane
void TASInputDlg::UpdateExtraButtons(bool check, bool uncheck)
{
	if (m_rollassist.checkbox && m_has_layout && !Movie::IsPlayingInput())
	{
		if (check)
		{
			m_rollassist.checkbox->SetValue(true);
		}
		else if (uncheck)
		{
			m_rollassist.checkbox->SetValue(false);
		}
	}
}

void TASInputDlg::UpdateFromButtons(wxCommandEvent& event)
{
	if (m_rollassist.checkbox && m_has_layout && !Movie::IsPlayingInput())
	{
		if (m_rollassist.checkbox->IsChecked() && !Movie::roll_enabled)
		{
			Movie::roll_enabled = true;
			Movie::first_roll = true;
			Movie::roll_timer = -1;
		}
		if (!m_rollassist.checkbox->IsChecked())
		{
			Movie::roll_enabled = false;
		}
	}
	else
	{
		Movie::roll_enabled = false;
	}

	Movie::checkSave = false;
	Movie::uncheckSave = false;
}


//Dragonbane: Lua Wrapper Functions
void TASInputDlg::iPressButton(const char* button)
{
	if (!strcmp(button, "A"))
		m_a.checkbox->SetValue(true);
	else if (!strcmp(button, "B"))
		m_b.checkbox->SetValue(true);
	else if (!strcmp(button, "X"))
		m_x.checkbox->SetValue(true);
	else if (!strcmp(button, "Y"))
		m_y.checkbox->SetValue(true);
	else if (!strcmp(button, "Z"))
		m_z.checkbox->SetValue(true);
	else if (!strcmp(button, "L"))
		m_l.checkbox->SetValue(true);
	else if (!strcmp(button, "R"))
		m_r.checkbox->SetValue(true);
	else if (!strcmp(button, "Start"))
		m_start.checkbox->SetValue(true);
	else if (!strcmp(button, "D-Up"))
		m_dpad_up.checkbox->SetValue(true);
	else if (!strcmp(button, "D-Down"))
		m_dpad_down.checkbox->SetValue(true);
	else if (!strcmp(button, "D-Left"))
		m_dpad_left.checkbox->SetValue(true);
	else if (!strcmp(button, "D-Right"))
		m_dpad_right.checkbox->SetValue(true);
}
void TASInputDlg::iReleaseButton(const char* button)
{
	if (!strcmp(button, "A"))
		m_a.checkbox->SetValue(false);
	else if (!strcmp(button, "B"))
		m_b.checkbox->SetValue(false);
	else if (!strcmp(button, "X"))
		m_x.checkbox->SetValue(false);
	else if (!strcmp(button, "Y"))
		m_y.checkbox->SetValue(false);
	else if (!strcmp(button, "Z"))
		m_z.checkbox->SetValue(false);
	else if (!strcmp(button, "L"))
		m_l.checkbox->SetValue(false);
	else if (!strcmp(button, "R"))
		m_r.checkbox->SetValue(false);
	else if (!strcmp(button, "Start"))
		m_start.checkbox->SetValue(false);
	else if (!strcmp(button, "D-Up"))
		m_dpad_up.checkbox->SetValue(false);
	else if (!strcmp(button, "D-Down"))
		m_dpad_down.checkbox->SetValue(false);
	else if (!strcmp(button, "D-Left"))
		m_dpad_left.checkbox->SetValue(false);
	else if (!strcmp(button, "D-Right"))
		m_dpad_right.checkbox->SetValue(false);
}
void TASInputDlg::iSetMainStickX(int xVal)
{
	m_main_stick.x_cont.text->SetValue(std::to_string(xVal));
}
void TASInputDlg::iSetMainStickY(int yVal)
{
	m_main_stick.y_cont.text->SetValue(std::to_string(yVal));
}
void TASInputDlg::iSetCStickX(int xVal)
{
	m_c_stick.x_cont.text->SetValue(std::to_string(xVal));
}
void TASInputDlg::iSetCStickY(int yVal)
{
	m_c_stick.y_cont.text->SetValue(std::to_string(yVal));
}


//Dragonbane: Custom Scripts
void TASInputDlg::ExecuteScripts()
{
	if (!Core::IsRunningAndStarted())
		return;

	std::string gameID = SConfig::GetInstance().m_LocalCoreStartupParameter.GetUniqueID();
	u32 isLoadingAdd;
	u32 eventFlagAdd;
	u32 charPointerAdd;
	bool isTP = false;

	if (!gameID.compare("GZ2E01"))
	{
		eventFlagAdd = 0x40b16d;
		isLoadingAdd = 0x450ce0;

		isTP = true;
	}
	else if (!gameID.compare("GZ2P01"))
	{
		eventFlagAdd = 0x40d10d;
		isLoadingAdd = 0x452ca0;

		isTP = true;
	}

	//TWW Stuff
	bool isTWW = false;

	if (!gameID.compare("GZLJ01"))
	{
		isLoadingAdd = 0x3ad335;
		eventFlagAdd = 0x3bd3a2;
		charPointerAdd = 0x3ad860;

		isTWW = true;
	}

	//Reset button
	if (m_reset.checkbox->IsChecked())
	{
		m_reset.checkbox->SetValue(false);

		if (Movie::IsRecordingInput())
			Movie::g_bReset = true;

		ProcessorInterface::ResetButton_Tap();
	}

	//Auto Dialog
	if (m_skipDialog.checkbox->IsChecked())
	{
		if (!auto_dialog)
		{
			dialog_timer = Movie::g_currentFrame;
			auto_dialog = true;
		}

		if (dialog_timer + 2 == Movie::g_currentFrame)
		{
			dialog_timer = Movie::g_currentFrame;
		}

		if (dialog_timer + 1 == Movie::g_currentFrame)
		{
			m_b.checkbox->SetValue(false);
			m_a.checkbox->SetValue(true);
		}

		if (dialog_timer == Movie::g_currentFrame)
		{
			m_b.checkbox->SetValue(true);
			m_a.checkbox->SetValue(false);
		}
	}
	else
	{
		auto_dialog = false;
	}

	//Quickspin
	if (m_quickspin.checkbox->IsChecked())
	{
		m_main_stick.x_cont.text->SetValue(std::to_string(194));
		m_main_stick.y_cont.text->SetValue(std::to_string(191));

		quickspin_enabled = true;
		quickspin_timer = Movie::g_currentFrame;
		m_quickspin.checkbox->SetValue(false);
	}
	else if (quickspin_enabled)
	{
		if (Movie::g_currentFrame == quickspin_timer + 1)
		{
			m_main_stick.x_cont.text->SetValue(std::to_string(208));
			m_main_stick.y_cont.text->SetValue(std::to_string(81));
		}
		else if (Movie::g_currentFrame == quickspin_timer + 2)
		{
			m_main_stick.x_cont.text->SetValue(std::to_string(122));
			m_main_stick.y_cont.text->SetValue(std::to_string(31));
		}
		else if (Movie::g_currentFrame == quickspin_timer + 3)
		{
			m_main_stick.x_cont.text->SetValue(std::to_string(50));
			m_main_stick.y_cont.text->SetValue(std::to_string(89));
		}
		else if (Movie::g_currentFrame == quickspin_timer + 4)
		{
			m_main_stick.x_cont.text->SetValue(std::to_string(76));
			m_main_stick.y_cont.text->SetValue(std::to_string(193));
		}
		else if (Movie::g_currentFrame == quickspin_timer + 5)
		{
			m_main_stick.x_cont.text->SetValue(std::to_string(198));
			m_main_stick.y_cont.text->SetValue(std::to_string(185));
			m_b.checkbox->SetValue(true);
		}
		else if (Movie::g_currentFrame == quickspin_timer + 6)
		{
			m_main_stick.x_cont.text->SetValue(std::to_string(128));
			m_main_stick.y_cont.text->SetValue(std::to_string(128));
			m_b.checkbox->SetValue(false);

			quickspin_enabled = false;
		}
	}

	//Roll Assist
	if (Movie::checkSave)
	{
		m_rollassist.checkbox->SetValue(true);
		Movie::checkSave = false;
		Movie::uncheckSave = false;
	}
	else if (Movie::uncheckSave)
	{
		m_rollassist.checkbox->SetValue(false);
		Movie::checkSave = false;
		Movie::uncheckSave = false;
	}

	if (m_rollassist.checkbox->IsChecked() && !Movie::roll_enabled)
	{
		if (isTP || isTWW)
		{
			Movie::roll_enabled = true;
			Movie::roll_timer = Movie::g_currentFrame;
		}

		if (isTP)
			Movie::first_roll = true;
		else
			Movie::first_roll = false;
	}
	if (!m_rollassist.checkbox->IsChecked())
	{
		Movie::roll_enabled = false;
	}
	if (Movie::roll_enabled)
	{
		if (Movie::roll_timer == -1)
			Movie::roll_timer = Movie::g_currentFrame;

		if (isTP)
		{
			u8 eventFlag = Memory::Read_U8(eventFlagAdd);
			u32 isLoading = Memory::Read_U32(isLoadingAdd);

			if (eventFlag == 1 || isLoading > 0 || m_main_stick.x_cont.value == 128 && m_main_stick.y_cont.value == 128) //Reset rolling during loading or cutscenes
			{
				Movie::first_roll = true;
				Movie::roll_timer = Movie::g_currentFrame + 1;
				m_a.checkbox->SetValue(false);
			}
			if (Movie::first_roll)
			{
				if (Movie::g_currentFrame == Movie::roll_timer)
				{
					m_a.checkbox->SetValue(true);
				}
				else if (Movie::g_currentFrame == Movie::roll_timer + 1)
				{
					m_a.checkbox->SetValue(false);
				}
				else if (Movie::g_currentFrame == Movie::roll_timer + 23) //22 would be ideal, but terrain...
				{
					m_a.checkbox->SetValue(true);
				}
				else if (Movie::g_currentFrame == Movie::roll_timer + 24)
				{
					m_a.checkbox->SetValue(false);
					Movie::first_roll = false;
					Movie::roll_timer = Movie::g_currentFrame - 1;
				}
			}
			else
			{
				if (Movie::g_currentFrame == Movie::roll_timer + 20) //19 would be ideal, but terrain...
				{
					m_a.checkbox->SetValue(true);
				}
				else if (Movie::g_currentFrame == Movie::roll_timer + 21)
				{
					m_a.checkbox->SetValue(false);
					Movie::roll_timer = Movie::g_currentFrame - 1;
				}
			}
		}

		if (isTWW)
		{
			u8 eventFlag = Memory::Read_U8(eventFlagAdd);
			u8 isLoading = Memory::Read_U8(isLoadingAdd);
			u32 characterpointer = Memory::Read_U32(charPointerAdd);

			if (characterpointer > 0x80000000)
			{
				characterpointer -= 0x80000000;

				float LinkSpeed = Memory::Read_F32(characterpointer + 0x34e4);

				if (isLoading > 0 || m_main_stick.x_cont.value == 128 && m_main_stick.y_cont.value == 128 || LinkSpeed < 13.590374) //Reset rolling during loading or idle or not enough speed
				{
					Movie::first_roll = false;
					Movie::roll_timer = Movie::g_currentFrame + 1;
					m_a.checkbox->SetValue(false);
				}
				else
				{
					if (Movie::g_currentFrame == Movie::roll_timer)
					{
						m_a.checkbox->SetValue(true);
					}
					else if (Movie::g_currentFrame == Movie::roll_timer + 1)
					{
						m_a.checkbox->SetValue(false);
						Movie::roll_timer = Movie::g_currentFrame + 15; //16 to 15
					}
				}
			}
			else
			{
				Movie::first_roll = false;
				Movie::roll_timer = Movie::g_currentFrame + 1;
				m_a.checkbox->SetValue(false);
			}
		}
	}

	//Superswim Script
	if (Movie::swimStarted && !Movie::swimInProgress) //Start Superswim
	{
		//Dragonbane: Give instance of class
		luaInstance = this;

		luaState = luaL_newstate();

		luaL_openlibs(luaState);

		//Make functions available to Lua programs
		lua_register(luaState, "ReadValue8", ReadValue8);
		lua_register(luaState, "ReadValue16", ReadValue16);
		lua_register(luaState, "ReadValue32", ReadValue32);
		lua_register(luaState, "ReadValueFloat", ReadValueFloat);
		lua_register(luaState, "ReadValueString", ReadValueString);
		lua_register(luaState, "GetPointerNormal", GetPointerNormal);

		lua_register(luaState, "PressButton", PressButton);
		lua_register(luaState, "ReleaseButton", ReleaseButton);
		lua_register(luaState, "SetMainStickX", SetMainStickX);
		lua_register(luaState, "SetMainStickY", SetMainStickY);
		lua_register(luaState, "SetCStickX", SetCStickX);
		lua_register(luaState, "SetCStickY", SetCStickY);

		lua_register(luaState, "GetFrameCount", GetFrameCount);
		lua_register(luaState, "MsgBox", MsgBox);
		lua_register(luaState, "AbortSwim", AbortSwim);

		std::string file = File::GetExeDirectory() + "\\Scripts\\Superswim.lua";

		int status = luaL_dofile(luaState, file.c_str());

		if (status == 0)
		{
			//Execute Start function
			lua_getglobal(luaState, "startSwim");

			lua_pushnumber(luaState, Movie::swimDestPosX);
			lua_pushnumber(luaState, Movie::swimDestPosZ);

			status = lua_pcall(luaState, 2, LUA_MULTRET, 0);
		}
		
		if (status != 0)
		{
			HandleLuaErrors(luaState, status);
			lua_close(luaState);

			Movie::swimStarted = false;
		}

		Movie::swimInProgress = true;
	}
	else if (!Movie::swimStarted && Movie::swimInProgress) 	//Cancel Superswim
	{
		lua_getglobal(luaState, "cancelSwim");

		int status = lua_pcall(luaState, 0, LUA_MULTRET, 0);

		if (status != 0)
		{
			HandleLuaErrors(luaState, status);
		}

		lua_close(luaState);

		Movie::swimInProgress = false;
	}
	else if (Movie::swimStarted && Movie::swimInProgress) //Call Update function
	{ 
		lua_getglobal(luaState, "updateSwim");

		int status = lua_pcall(luaState, 0, LUA_MULTRET, 0);

		if (status != 0)
		{
			HandleLuaErrors(luaState, status);

			lua_close(luaState);

			Movie::swimInProgress = false;
			Movie::swimStarted = false;
		}
	}
}


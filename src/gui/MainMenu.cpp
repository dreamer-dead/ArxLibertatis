/*
 * Copyright 2014 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/MainMenu.h"

#include <iomanip>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "audio/Audio.h"
#include "core/Application.h"
#include "core/Config.h"
#include "core/Core.h"
#include "core/Localisation.h"
#include "core/SaveGame.h"
#include "core/Version.h"
#include "gui/MenuWidgets.h"
#include "gui/MenuPublic.h"
#include "gui/Hud.h"
#include "gui/Text.h"
#include "gui/TextManager.h"
#include "gui/widget/CheckboxWidget.h"
#include "gui/widget/CycleTextWidget.h"
#include "gui/widget/HorizontalPanelWidget.h"
#include "gui/widget/SliderWidget.h"
#include "graphics/Draw.h"
#include "graphics/Math.h"
#include "graphics/Renderer.h"
#include "graphics/font/Font.h"
#include "graphics/data/TextureContainer.h"
#include "input/Input.h"
#include "input/Keyboard.h"
#include "window/RenderWindow.h"

CWindowMenu * pWindowMenu = NULL;
TextWidget * pDeleteConfirm = NULL;
TextWidget * pLoadConfirm = NULL;
TextWidget * pDeleteButton = NULL;
TextWidget * pMenuElementApply = NULL;

extern MainMenu *mainMenu;

class NewQuestMenuPage : public MenuPage {
public:
	NewQuestMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, NEW_QUEST)
	{}

	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_confirm");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText);
			me->SetCheckOff();
			addCenter(me, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_main_newquest_confirm");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText);
			me->SetCheckOff();
			addCenter(me, true);
		}
		
		HorizontalPanelWidget * pPanel = new HorizontalPanelWidget;
		
		{
			std::string szMenuText = getLocalised("system_yes");
			szMenuText += "   "; // TODO This space can probably go
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText);
			me->clicked = boost::bind(ARXMenu_NewQuest);
			me->SetPos(Vec2i(RATIO_X(m_size.x - (me->m_rect.width() + 10)), 0));
			pPanel->AddElementNoCenterIn(me);
		}
		
		{
			std::string szMenuText = getLocalised("system_no");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(10), 0));
			me->m_targetMenu = MAIN;
			me->SetShortCut(Keyboard::Key_Escape);
			pPanel->AddElementNoCenterIn(me);
		}
		
		pPanel->Move(Vec2i(0, RATIO_Y(380)));
		
		add(pPanel);
	}
};

class ChooseLoadOrSaveMenuPage : public MenuPage {
public:
	ChooseLoadOrSaveMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, EDIT_QUEST)
	{}
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_load");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(0, 0));
			me->clicked = boost::bind(&ChooseLoadOrSaveMenuPage::onClickLoad, this);
			me->m_targetMenu = EDIT_QUEST_LOAD;
			me->m_savegame = SavegameHandle::Invalid;
			addCenter(me, true);
		}
		
		{
			std::string szMenuText = getLocalised( "system_menus_main_editquest_save");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(0, 0));
			me->m_targetMenu = EDIT_QUEST_SAVE;
			
			if(!ARXMenu_CanResumeGame()) {
				me->SetCheckOff();
				me->lColor = Color(127, 127, 127);
			}
			addCenter(me, true);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 380)), "graph/interface/menus/back");
			cb->m_targetMenu = MAIN;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
	void onClickLoad() {
		if(pWindowMenu)
		for(size_t i = 0; i < pWindowMenu->m_pages.size(); i++) {
			MenuPage * page = pWindowMenu->m_pages[i];
			
			if(page->eMenuState == EDIT_QUEST_LOAD) {
				page->m_savegame = m_savegame;
				
				for(size_t j = 0; j < page->m_children.m_widgets.size(); j++) {
					Widget * widget = page->m_children.m_widgets[j];
					
					if(widget->m_id == BUTTON_MENUEDITQUEST_LOAD) {
						((TextWidget *)widget)->bSelected = false;
					}
				}
			}
		}
	}
};

class LoadMenuPage : public MenuPage {
public:
	LoadMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, EDIT_QUEST_LOAD)
	{}

	void init() {
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2i(0, 0), "graph/interface/icons/menu_main_load");
			cb->SetCheckOff();
			addCenter(cb, true);
		}
		
		std::string quicksaveName = getLocalised("system_menus_main_quickloadsave", "Quicksave");
		
		// TODO make this list scrollable
		// TODO align the date part to the right!
		
		{
		size_t quicksaveNum = 0;
		
		// Show quicksaves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(!save.quicksave) {
				continue;
			}
			
			std::ostringstream text;
			text << quicksaveName << ' ' << ++quicksaveNum << "   " << save.time;
			
			TextWidget * e = new TextWidget(BUTTON_MENUEDITQUEST_LOAD, hFontControls, text.str(), Vec2i(RATIO_X(20), 0));
			e->m_savegame = SavegameHandle(i);
			addCenter(e);
		}
		
		// Show regular saves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(save.quicksave) {
				continue;
			}
			
			std::string text = save.name +  "   " + save.time;
			
			TextWidget * e = new TextWidget(BUTTON_MENUEDITQUEST_LOAD, hFontControls, text, Vec2i(RATIO_X(20), 0));
			e->m_savegame = SavegameHandle(i);
			addCenter(e);
		}
		
		{
			TextWidget * confirm = new TextWidget(BUTTON_INVALID, hFontControls, " ", Vec2i(RATIO_X(20), 0));
			confirm->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			confirm->SetCheckOff();
			confirm->m_savegame = SavegameHandle::Invalid;
			addCenter(confirm);
		}
		
		// Delete button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_delete");
			szMenuText += "   ";
			TextWidget * me = new TextWidget(BUTTON_MENUEDITQUEST_DELETE_CONFIRM, hFontMenu, szMenuText, Vec2i(0, 0));
			me->m_targetMenu = EDIT_QUEST_LOAD;
			me->SetPos(Vec2i(RATIO_X(m_size.x-10)-me->m_rect.width(), RATIO_Y(42)));
			me->SetCheckOff();
			me->lOldColor = me->lColor;
			me->lColor = Color::grayb(127);
			add(me);
			pDeleteConfirm = me;
		}
		
		// Load button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_load");
			szMenuText += "   ";
			TextWidget * me = new TextWidget(BUTTON_MENUEDITQUEST_LOAD_CONFIRM, hFontMenu, szMenuText, Vec2i(0, 0));
			me->m_targetMenu = MAIN;
			me->SetPos(Vec2i(RATIO_X(m_size.x-10)-me->m_rect.width(), RATIO_Y(380) + RATIO_Y(40)));
			me->SetCheckOff();
			me->lOldColor = me->lColor;
			me->lColor = Color::grayb(127);
			add(me);
			pLoadConfirm = me;
		}
		
		// Back button
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 420)), "graph/interface/menus/back");
			cb->clicked = boost::bind(&LoadMenuPage::onClickBack, this);
			cb->m_targetMenu = EDIT_QUEST;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
		}
	}
	
	void onClickBack() {
		pLoadConfirm->SetCheckOff();
		pLoadConfirm->lColor = Color::grayb(127);
		pDeleteConfirm->SetCheckOff();
		pDeleteConfirm->lColor = Color::grayb(127);
	}
};

class SaveMenuPage : public MenuPage {
public:
	SaveMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, EDIT_QUEST_SAVE)
	{}
	
	void init() {
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2i(RATIO_X(10), 0), "graph/interface/icons/menu_main_save");
			cb->SetCheckOff();
			addCenter(cb, true);
		}
		
		std::string quicksaveName = getLocalised("system_menus_main_quickloadsave", "Quicksave");
		size_t quicksaveNum = 0;
		
		// Show quicksaves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(!save.quicksave) {
				continue;
			}
			
			std::ostringstream text;
			text << quicksaveName << ' ' << ++quicksaveNum << "   " << save.time;
			
			TextWidget * e = new TextWidget(BUTTON_MENUEDITQUEST_SAVEINFO, hFontControls, text.str(), Vec2i(RATIO_X(20), 0.f));
			e->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			e->setColor(Color::grayb(127));
			e->SetCheckOff();
			e->m_savegame = SavegameHandle(i);
			addCenter(e);
		}
		
		// Show regular saves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(save.quicksave) {
				continue;
			}
			
			std::string text = save.name +  "   " + save.time;
			
			TextWidget * e = new TextWidget(BUTTON_MENUEDITQUEST_SAVEINFO, hFontControls, text, Vec2i(RATIO_X(20), 0.f));
			e->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			e->m_savegame = SavegameHandle(i);
			addCenter(e);
		}
		
		for(size_t i = savegames.size(); i <= 15; i++) {
			
			std::ostringstream text;
			text << '-' << std::setfill('0') << std::setw(4) << i << '-';
			
			TextWidget * e = new TextWidget(BUTTON_MENUEDITQUEST_SAVEINFO, hFontControls, text.str(), Vec2i(RATIO_X(20), 0));
			e->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			e->m_savegame = SavegameHandle::Invalid;
			addCenter(e);
		}
	
		{
			TextWidget * me01 = new TextWidget(BUTTON_INVALID, hFontControls, " ", Vec2i(RATIO_X(20), 0));
			me01->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			me01->m_savegame = SavegameHandle::Invalid;
			me01->SetCheckOff();
			addCenter(me01);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 420)), "graph/interface/menus/back");
			cb->m_targetMenu = EDIT_QUEST;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
};

class SaveConfirmMenuPage : public MenuPage {
public:
	SaveConfirmMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, EDIT_QUEST_SAVE_CONFIRM)
	{}
	
	void init() {
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2i(0, 0), "graph/interface/icons/menu_main_save");
			cb->SetCheckOff();
			addCenter(cb, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menu_editquest_newsavegame", "---");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			me->m_savegame = SavegameHandle::Invalid;
			me->eState=EDIT;
			me->ePlace=CENTER;
			addCenter(me, true);
		}
		
		HorizontalPanelWidget * pPanel = new HorizontalPanelWidget;
		
		// Delete button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_delete");
			TextWidget * me = new TextWidget(BUTTON_MENUEDITQUEST_DELETE, hFontMenu, szMenuText, Vec2i(0, 0));
			me->m_targetMenu = EDIT_QUEST_SAVE;
			me->SetPos(Vec2i(RATIO_X(m_size.x-10)-me->m_rect.width(), RATIO_Y(5)));
			me->lOldColor = me->lColor;
			pPanel->AddElementNoCenterIn(me);
			pDeleteButton = me;
		}
		
		// Save button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_save");
			TextWidget * me = new TextWidget(BUTTON_MENUEDITQUEST_SAVE, hFontMenu, szMenuText, Vec2i(0, 0));
			me->m_targetMenu = MAIN;
			me->SetPos(Vec2i(RATIO_X(m_size.x-10)-me->m_rect.width(), RATIO_Y(380)));
			pPanel->AddElementNoCenterIn(me);
		}
		
		// Back button
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 380)), "graph/interface/menus/back");
			cb->m_targetMenu = EDIT_QUEST_SAVE;
			cb->SetShortCut(Keyboard::Key_Escape);
			pPanel->AddElementNoCenterIn(cb);
		}
		
		add(pPanel);
	}
};

int newWidth;
int newHeight;
bool newFullscreen;

class OptionsMenuPage : public MenuPage {
public:
	OptionsMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, OPTIONS)
	{}
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(0, 0));
			me->clicked = boost::bind(&OptionsMenuPage::onClickedVideo, this);
			me->m_targetMenu = OPTIONS_VIDEO;
			addCenter(me, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_audio");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(0, 0));
			me->m_targetMenu = OPTIONS_AUDIO;
			addCenter(me, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(0, 0));
			me->m_targetMenu = OPTIONS_INPUT;
			addCenter(me, true);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 380)), "graph/interface/menus/back");
			cb->m_targetMenu = MAIN;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
	void onClickedVideo() {
		newWidth = config.video.resolution.x;
		newHeight = config.video.resolution.y;
		newFullscreen = config.video.fullscreen;
	}
};

// TODO remove this
const std::string AUTO_RESOLUTION_STRING = "Automatic";

class VideoOptionsMenuPage : public MenuPage {
public:
	VideoOptionsMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, OPTIONS_VIDEO)
	{
		fullscreenCheckbox = NULL;
		pMenuSliderResol = NULL;
	}
	
	CheckboxWidget * fullscreenCheckbox;
	CycleTextWidget * pMenuSliderResol;
	
	void init() {
		
		// Renderer selection
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_renderer", "Renderer");
			szMenuText += "  ";
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			me->SetCheckOff();
			pc->AddElement(me);
			CycleTextWidget * slider = new CycleTextWidget;
			slider->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedRenderer, this, _1, _2);
			
			{
				TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, "Auto-Select", Vec2i(0, 0));
				slider->AddText(text);
				slider->selectLast();
			}
			
	#if ARX_HAVE_SDL1 || ARX_HAVE_SDL2
			{
				TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, "OpenGL", Vec2i(0, 0));
				slider->AddText(text);
				if(config.window.framework == "SDL") {
					slider->selectLast();
				}
			}
	#endif
			
			float fRatio    = (RATIO_X(m_size.x-9) - slider->m_rect.width());
			slider->Move(Vec2i(checked_range_cast<int>(fRatio), 0));
			pc->AddElement(slider);
			addCenter(pc);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_videos_full_screen");
			if(szMenuText.empty()) {
				// TODO once we ship our own amendmends to the loc files a cleaner
				// fix would be to just define system_menus_options_videos_full_screen
				// for the german version there
				szMenuText = getLocalised("system_menus_options_video_full_screen");
			}
			szMenuText += "  ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			text->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedFullscreen, this, _1);
			cb->iState = config.video.fullscreen ? 1 : 0;
			addCenter(cb);
			fullscreenCheckbox = cb;
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_resolution");
			szMenuText += "  ";
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			me->SetCheckOff();
			pc->AddElement(me);
			pMenuSliderResol = new CycleTextWidget;
			pMenuSliderResol->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedResolution, this, _1, _2);
			
			pMenuSliderResol->setEnabled(config.video.fullscreen);
			
			const RenderWindow::DisplayModes & modes = mainApp->getWindow()->getDisplayModes();
			for(size_t i = 0; i != modes.size(); ++i) {
				
				const DisplayMode & mode = modes[i];
				
				// find the aspect ratio
				unsigned a = mode.resolution.x;
				unsigned b = mode.resolution.y;
				while(b != 0) {
					unsigned t = a % b;
					a = b, b = t;
				}
				Vec2i aspect = mode.resolution / Vec2i(a);
				
				std::stringstream ss;
				ss << mode;
				
				if(aspect.x < 100 && aspect.y < 100) {
					if(aspect == Vec2i(8, 5)) {
						aspect = Vec2i(16, 10);
					}
					ss << " (" << aspect.x << ':' << aspect.y << ')';
				}
				
				pMenuSliderResol->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, ss.str()));
				
				if(mode.resolution == config.video.resolution) {
					pMenuSliderResol->selectLast();
				}
			}
			
			pMenuSliderResol->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, AUTO_RESOLUTION_STRING));
			
			if(config.video.resolution == Vec2i_ZERO) {
				pMenuSliderResol->selectLast();
			}
		
			float fRatio    = (RATIO_X(m_size.x-9) - pMenuSliderResol->m_rect.width()); 
		
			pMenuSliderResol->Move(Vec2i(checked_range_cast<int>(fRatio), 0));
			
			pc->AddElement(pMenuSliderResol);
			addCenter(pc);
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_detail");
			szMenuText += " ";
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			me->SetCheckOff();
			pc->AddElement(me);
			
			CycleTextWidget * cb = new CycleTextWidget;
			cb->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedQuality, this, _1, _2);
			szMenuText = getLocalised("system_menus_options_video_texture_low");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_video_texture_med");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_video_texture_high");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			cb->setValue(config.video.levelOfDetail);
			
			cb->Move(Vec2i(RATIO_X(m_size.x-9) - cb->m_rect.width(), 0));
			pc->AddElement(cb);
			
			addCenter(pc);
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_brouillard");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			me->SetCheckOff();
			pc->AddElement(me);
			SliderWidget * sld = new SliderWidget(Vec2i(RATIO_X(200), 0));
			sld->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedFogDistance, this, _1);
			sld->setValue(config.video.fogDistance);
			pc->AddElement(sld);
			addCenter(pc);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video_crosshair", "Show Crosshair");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			text->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedCrosshair, this, _1);
			cb->iState = config.video.showCrosshair ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video_antialiasing", "antialiasing");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			text->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedAntialiasing, this, _1);
			cb->iState = config.video.antialiasing ? 1 : 0;
			addCenter(cb);
		}
		
		ARX_SetAntiAliasing();
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video_vsync", "VSync");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			text->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedVsync, this, _1);
			cb->iState = config.video.vsync ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video_hud_scale", "Scale Hud");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			text->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedHudScale, this, _1);
			cb->iState = config.video.hudScale ? 1 : 0;
			addCenter(cb);
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_video_apply");
			szMenuText += "   ";
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(240), 0));
			me->clicked = boost::bind(&VideoOptionsMenuPage::onClickedApply, this);
			me->SetPos(Vec2i(RATIO_X(m_size.x-10)-me->m_rect.width(), RATIO_Y(380) + RATIO_Y(40)));
			me->SetCheckOff();
			pc->AddElementNoCenterIn(me);
			pMenuElementApply = me;
			
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 420)), "graph/interface/menus/back");
			cb->clicked = boost::bind(&VideoOptionsMenuPage::onClickedBack, this);
			cb->m_targetMenu = OPTIONS;
			cb->SetShortCut(Keyboard::Key_Escape);
			pc->AddElementNoCenterIn(cb);
			
			add(pc);
		}
	}
	
	
	void onChangedRenderer(int pos, const std::string & str) {
		ARX_UNUSED(str);
		
		switch(pos) {
			case 0:  config.window.framework = "auto"; break;
			case 1:  config.window.framework = "SDL";  break;
			default: config.window.framework = "auto"; break;
		}
	}
	
	void onChangedFullscreen(int state) {
		newFullscreen = ((state)?true:false);
		
		if(pMenuSliderResol) {
			pMenuSliderResol->setEnabled(newFullscreen);
		}
	}
	
	void onChangedResolution(int pos, const std::string & str) {
		ARX_UNUSED(pos);
		
		if(str == AUTO_RESOLUTION_STRING) {
			newWidth = newHeight = 0;
		} else {
			std::stringstream ss(str);
			int iX = config.video.resolution.x;
			int iY = config.video.resolution.y;
			char tmp;
			ss >> iX >> tmp >> iY;
			newWidth = iX;
			newHeight = iY;
		}
	}
	
	void onChangedQuality(int pos, const std::string & str) {
		ARX_UNUSED(str);
		
		ARXMenu_Options_Video_SetDetailsQuality(pos);
	}
	
	void onChangedFogDistance(int value) {
		ARXMenu_Options_Video_SetFogDistance(value);
	}
	
	void onChangedCrosshair(int state) {
		config.video.showCrosshair = state ? true : false;
	}
	
	void onChangedAntialiasing(int state) {
		config.video.antialiasing = state ? true : false;
		ARX_SetAntiAliasing();
	}
	
	void onChangedVsync(int state) {
		config.video.vsync = state ? true : false;
	}
	
	void onChangedHudScale(int state) {
		config.video.hudScale = state ? true : false;
		g_hudRoot.recalcScale();
	}
	
	void onClickedBack() {
		if(pMenuSliderResol && pMenuSliderResol->getOldValue() >= 0) {
			pMenuSliderResol->setValue(pMenuSliderResol->getOldValue());
			pMenuSliderResol->setOldValue(-1);
			newWidth=config.video.resolution.x;
			newHeight=config.video.resolution.y;
		}
		
		if(fullscreenCheckbox && fullscreenCheckbox->iOldState >= 0) {
			fullscreenCheckbox->iState = fullscreenCheckbox->iOldState;
			fullscreenCheckbox->iOldState = -1;
			newFullscreen = config.video.fullscreen;
		}
	}
	
	void onClickedApply() {
		if(newWidth != config.video.resolution.x
		   || newHeight!=config.video.resolution.y
		   || newFullscreen != config.video.fullscreen
		) {
			ARXMenu_Private_Options_Video_SetResolution(newFullscreen, newWidth, newHeight);
			pMenuSliderResol->setOldValue(-1);
			fullscreenCheckbox->iOldState = -1;
		}
		mainMenu->bReInitAll=true;
	}
};

class AudioOptionsMenuPage : public MenuPage {
public:
	AudioOptionsMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, OPTIONS_AUDIO)
	{}
	
	void init() {
		
		// Audio backend selection
		{
			
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_device", "Device");
			szMenuText += "  ";
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			me->SetCheckOff();
			pc->AddElement(me);
			CycleTextWidget * slider = new CycleTextWidget;
			slider->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedDevice, this, _1, _2);
			
			int maxwidth = RATIO_X(m_size.x - 28) - me->m_rect.width() - slider->m_rect.width();
			
			slider->AddText(new TextWidget(BUTTON_INVALID, hFontControls, "Default"));
			slider->selectLast();
			
			BOOST_FOREACH(const std::string & device, audio::getDevices()) {
				TextWidget * text = new TextWidget(BUTTON_INVALID, hFontControls, device);
				if(text->m_rect.width() > maxwidth) {
					text->m_rect.right = text->m_rect.left + maxwidth;
				}
				slider->AddText(text);
				if(config.audio.device == device) {
					slider->selectLast();
				}
			}
			
			float fRatio    = (RATIO_X(m_size.x-9) - slider->m_rect.width());
			slider->Move(Vec2i(checked_range_cast<int>(fRatio), 0));
			pc->AddElement(slider);
			addCenter(pc);
			
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_master_volume");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			me->SetCheckOff();
			pc->AddElement(me);
			SliderWidget * sld = new SliderWidget(Vec2i(RATIO_X(200), 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedMasterVolume, this, _1);
			sld->setValue((int)config.audio.volume); // TODO use float sliders
			pc->AddElement(sld);
			addCenter(pc);
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_effects_volume");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			me->m_targetMenu = OPTIONS_AUDIO;
			me->SetCheckOff();
			pc->AddElement(me);
			SliderWidget * sld = new SliderWidget(Vec2i(RATIO_X(200), 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedEffectsVolume, this, _1);
			sld->setValue((int)config.audio.sfxVolume);
			pc->AddElement(sld);
			addCenter(pc);
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_speech_volume");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			me->m_targetMenu = OPTIONS_AUDIO;
			me->SetCheckOff();
			pc->AddElement(me);
			SliderWidget * sld = new SliderWidget(Vec2i(RATIO_X(200), 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedSpeechVolume, this, _1);
			sld->setValue((int)config.audio.speechVolume);
			pc->AddElement(sld);
			addCenter(pc);
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_ambiance_volume");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			me->m_targetMenu = OPTIONS_AUDIO;
			me->SetCheckOff();
			pc->AddElement(me);
			SliderWidget * sld = new SliderWidget(Vec2i(RATIO_X(200), 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedAmbianceVolume, this, _1);
			sld->setValue((int)config.audio.ambianceVolume);
			pc->AddElement(sld);
			addCenter(pc);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_audio_eax", "EAX");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			text->m_targetMenu = OPTIONS_INPUT;
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&AudioOptionsMenuPage::onChangedEax, this, _1);
			if(audio::isReverbSupported()) {
				cb->iState = config.audio.eax ? 1 : 0;
			} else {
				cb->SetCheckOff();
			}
			addCenter(cb);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 380)), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
	
	void onChangedDevice(int pos, const std::string & str) {
		if(pos == 0) {
			ARXMenu_Options_Audio_SetDevice("auto");
		} else {
			ARXMenu_Options_Audio_SetDevice(str);
		}
	}
	
	void onChangedMasterVolume(int value) {
		ARXMenu_Options_Audio_SetMasterVolume(value);
	}
	
	void onChangedEffectsVolume(int value) {
		ARXMenu_Options_Audio_SetSfxVolume(value);
	}
	
	void onChangedSpeechVolume(int value) {
		ARXMenu_Options_Audio_SetSpeechVolume(value);
	}
	
	void onChangedAmbianceVolume(int value) {
		ARXMenu_Options_Audio_SetAmbianceVolume(value);
	}
	
	void onChangedEax(int state) {
		ARXMenu_Options_Audio_SetEAX(state != 0);
	}
};

class InputOptionsMenuPage : public MenuPage {
public:
	InputOptionsMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, OPTIONS_INPUT)
	{}
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input_customize_controls");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			me->m_targetMenu = OPTIONS_INPUT_CUSTOMIZE_KEYS_1;
			addCenter(me);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input_invert_mouse");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			text->m_targetMenu = OPTIONS_INPUT; // TODO is this correct ?
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedInvertMouse, this, _1);
			cb->iState = config.input.invertMouse ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_auto_ready_weapon");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			text->m_targetMenu = OPTIONS_INPUT; // TODO is this correct ?
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedAutoReadyWeapon, this, _1);
			cb->iState = config.input.autoReadyWeapon ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input_mouse_look_toggle");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			text->m_targetMenu = OPTIONS_INPUT; // TODO is this correct ?
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedToggleMouselook, this, _1);
			cb->iState = config.input.mouseLookToggle ? 1 : 0;
			addCenter(cb);
		}
		
		{
			HorizontalPanelWidget *pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_input_mouse_sensitivity");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0.f));
			me->SetCheckOff();
			pc->AddElement(me);
			SliderWidget * sld = new SliderWidget(Vec2i(RATIO_X(200), 0));
			sld->valueChanged = boost::bind(&InputOptionsMenuPage::onChangedMouseSensitivity, this, _1);
			sld->setValue(config.input.mouseSensitivity);
			pc->AddElement(sld);
			addCenter(pc);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_autodescription", "auto_description");
			szMenuText += " ";
			TextWidget * text = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			text->m_targetMenu = OPTIONS_INPUT; // TODO is this correct ?
			CheckboxWidget * cb = new CheckboxWidget(text);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedAutoDescription, this, _1);
			cb->iState = config.input.autoDescription ? 1 : 0;
			addCenter(cb);
		}
		
		{
			HorizontalPanelWidget * pc = new HorizontalPanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_misc_quicksave_slots", "Quicksave slots");
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2i(RATIO_X(20), 0));
			me->SetCheckOff();
			pc->AddElement(me);
			SliderWidget * sld = new SliderWidget(Vec2i(RATIO_X(200), 0));
			sld->setMinimum(1);
			sld->valueChanged = boost::bind(&InputOptionsMenuPage::onChangedQuicksaveSlots, this, _1);
			sld->setValue(config.misc.quicksaveSlots);
			pc->AddElement(sld);
			addCenter(pc);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 380)), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
	void onChangedInvertMouse(int state) {
		ARXMenu_Options_Control_SetInvertMouse((state)?true:false);
	}
	
	void onChangedAutoReadyWeapon(int state) {
		config.input.autoReadyWeapon = (state) ? true : false;
	}
	
	void onChangedToggleMouselook(int state) {
		config.input.mouseLookToggle = (state) ? true : false;
	}
	
	void onChangedMouseSensitivity(int value) {
		ARXMenu_Options_Control_SetMouseSensitivity(value);
	}
	
	void onChangedAutoDescription(int state) {
		config.input.autoDescription = (state) ? true : false;
	}
	
	void onChangedQuicksaveSlots(int value) {
		config.misc.quicksaveSlots = value;
	}
};


class ControlOptionsPage : public MenuPage {
public:
	ControlOptionsPage(Vec2i pos, Vec2i size, MENUSTATE state)
		: MenuPage(pos, size, state)
	{}
	
	void addControlRow(long & y,
	                             const std::string & a, MenuButton c, MenuButton d,
	                             const char * defaultText = "?",
	                             const char * specialSuffix = "") {
		
		HorizontalPanelWidget * pc = new HorizontalPanelWidget;
		
		std::string szMenuText = getLocalised(a, defaultText);
		szMenuText += specialSuffix;
		TextWidget * me = new TextWidget(BUTTON_INVALID, hFontControls, szMenuText, Vec2i(RATIO_X(20), 0));
		me->SetCheckOff();
		pc->AddElement(me);
		
		me = new TextWidget(c, hFontControls, "---", Vec2i(RATIO_X(150), 0));
		me->eState=GETTOUCH;
		pc->AddElement(me);
		
		me = new TextWidget(d, hFontControls, "---", Vec2i(RATIO_X(245), 0));
		me->eState=GETTOUCH;
		pc->AddElement(me);
		
		pc->Move(Vec2i(0, y));
		add(pc);
		y += pc->m_rect.height() + RATIO_Y(3.f);
	}
};


class ControlOptionsMenuPage1 : public ControlOptionsPage {
public:
	ControlOptionsMenuPage1(Vec2i pos, Vec2i size)
		: ControlOptionsPage(pos, size, OPTIONS_INPUT_CUSTOMIZE_KEYS_1)
	{}
	
	void init() {
		
		long y = static_cast<long>(RATIO_Y(8.f));
		
		addControlRow(y, "system_menus_options_input_customize_controls_mouselook", BUTTON_MENUOPTIONS_CONTROLS_CUST_USE1, BUTTON_MENUOPTIONS_CONTROLS_CUST_USE2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_action_combine", BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE1, BUTTON_MENUOPTIONS_CONTROLS_CUST_ACTIONCOMBINE2);
		addControlRow(y, "system_menus_options_input_customize_controls_jump", BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP1, BUTTON_MENUOPTIONS_CONTROLS_CUST_JUMP2);
		addControlRow(y, "system_menus_options_input_customize_controls_magic_mode", BUTTON_MENUOPTIONS_CONTROLS_CUST_MAGICMODE1, BUTTON_MENUOPTIONS_CONTROLS_CUST_MAGICMODE2);
		addControlRow(y, "system_menus_options_input_customize_controls_stealth_mode", BUTTON_MENUOPTIONS_CONTROLS_CUST_STEALTHMODE1, BUTTON_MENUOPTIONS_CONTROLS_CUST_STEALTHMODE2);
		addControlRow(y, "system_menus_options_input_customize_controls_walk_forward", BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKFORWARD1, BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKFORWARD2);
		addControlRow(y, "system_menus_options_input_customize_controls_walk_backward", BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKBACKWARD1, BUTTON_MENUOPTIONS_CONTROLS_CUST_WALKBACKWARD2);
		addControlRow(y, "system_menus_options_input_customize_controls_strafe_left", BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFELEFT1, BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFELEFT2);
		addControlRow(y, "system_menus_options_input_customize_controls_strafe_right", BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFERIGHT1, BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFERIGHT2);
		addControlRow(y, "system_menus_options_input_customize_controls_lean_left", BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANLEFT1, BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANLEFT2);
		addControlRow(y, "system_menus_options_input_customize_controls_lean_right", BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANRIGHT1, BUTTON_MENUOPTIONS_CONTROLS_CUST_LEANRIGHT2);
		addControlRow(y, "system_menus_options_input_customize_controls_crouch", BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCH1, BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCH2);
		addControlRow(y, "system_menus_options_input_customize_controls_crouch_toggle", BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCHTOGGLE1, BUTTON_MENUOPTIONS_CONTROLS_CUST_CROUCHTOGGLE2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_strafe", BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFE1, BUTTON_MENUOPTIONS_CONTROLS_CUST_STRAFE2);
		addControlRow(y, "system_menus_options_input_customize_controls_center_view", BUTTON_MENUOPTIONS_CONTROLS_CUST_CENTERVIEW1, BUTTON_MENUOPTIONS_CONTROLS_CUST_CENTERVIEW2);
		addControlRow(y, "system_menus_options_input_customize_controls_freelook", BUTTON_MENUOPTIONS_CONTROLS_CUST_FREELOOK1, BUTTON_MENUOPTIONS_CONTROLS_CUST_FREELOOK2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_turn_left", BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNLEFT1, BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNLEFT2);
		addControlRow(y, "system_menus_options_input_customize_controls_turn_right", BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNRIGHT1, BUTTON_MENUOPTIONS_CONTROLS_CUST_TURNRIGHT2);
		addControlRow(y, "system_menus_options_input_customize_controls_look_up", BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKUP1, BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKUP2);
		addControlRow(y, "system_menus_options_input_customize_controls_look_down", BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKDOWN1, BUTTON_MENUOPTIONS_CONTROLS_CUST_LOOKDOWN2);
		
		HorizontalPanelWidget * pc = new HorizontalPanelWidget;
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 380)), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS_INPUT;
			cb->SetShortCut(Keyboard::Key_Escape);
			pc->AddElementNoCenterIn(cb);
		}
		
		{
			std::string szMenuText = getLocalised( "system_menus_options_input_customize_default" );
			TextWidget * me = new TextWidget(BUTTON_MENUOPTIONS_CONTROLS_CUST_DEFAULT, hFontMenu, szMenuText);
			me->SetPos(Vec2i((RATIO_X(m_size.x) - me->m_rect.width())*0.5f, RATIO_Y(380)));
			pc->AddElementNoCenterIn(me);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(280, 380)), "graph/interface/menus/next");
			cb->m_targetMenu = OPTIONS_INPUT_CUSTOMIZE_KEYS_2;
			cb->SetShortCut(Keyboard::Key_Escape);
			pc->AddElementNoCenterIn(cb);
		}
	
		add(pc);
		ReInitActionKey();
	}
};

class ControlOptionsMenuPage2 : public ControlOptionsPage {
public:
	ControlOptionsMenuPage2(Vec2i pos, Vec2i size)
		: ControlOptionsPage(pos, size, OPTIONS_INPUT_CUSTOMIZE_KEYS_2)
	{}
	
	void init() {
		
		long y = static_cast<long>(RATIO_Y(8.f));
		
		addControlRow(y, "system_menus_options_input_customize_controls_inventory", BUTTON_MENUOPTIONS_CONTROLS_CUST_INVENTORY1, BUTTON_MENUOPTIONS_CONTROLS_CUST_INVENTORY2);
		addControlRow(y, "system_menus_options_input_customize_controls_book", BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOK1, BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOK2);
		addControlRow(y, "system_menus_options_input_customize_controls_bookcharsheet", BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKCHARSHEET1, BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKCHARSHEET2);
		addControlRow(y, "system_menus_options_input_customize_controls_bookmap", BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKMAP1, BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKMAP2);
		addControlRow(y, "system_menus_options_input_customize_controls_bookspell", BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKSPELL1, BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKSPELL2);
		addControlRow(y, "system_menus_options_input_customize_controls_bookquest", BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKQUEST1, BUTTON_MENUOPTIONS_CONTROLS_CUST_BOOKQUEST2);
		addControlRow(y, "system_menus_options_input_customize_controls_drink_potion_life", BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONLIFE1, BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONLIFE2);
		addControlRow(y, "system_menus_options_input_customize_controls_drink_potion_mana", BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONMANA1, BUTTON_MENUOPTIONS_CONTROLS_CUST_DRINKPOTIONMANA2);
		addControlRow(y, "system_menus_options_input_customize_controls_torch", BUTTON_MENUOPTIONS_CONTROLS_CUST_TORCH1, BUTTON_MENUOPTIONS_CONTROLS_CUST_TORCH2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_cancelcurrentspell", BUTTON_MENUOPTIONS_CONTROLS_CUST_CANCELCURSPELL1, BUTTON_MENUOPTIONS_CONTROLS_CUST_CANCELCURSPELL2);
		addControlRow(y, "system_menus_options_input_customize_controls_precast1", BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST1, BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST1_2);
		addControlRow(y, "system_menus_options_input_customize_controls_precast2", BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST2, BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST2_2);
		addControlRow(y, "system_menus_options_input_customize_controls_precast3", BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST3, BUTTON_MENUOPTIONS_CONTROLS_CUST_PRECAST3_2);
		addControlRow(y, "system_menus_options_input_customize_controls_weapon", BUTTON_MENUOPTIONS_CONTROLS_CUST_WEAPON1, BUTTON_MENUOPTIONS_CONTROLS_CUST_WEAPON2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_unequipweapon", BUTTON_MENUOPTIONS_CONTROLS_CUST_UNEQUIPWEAPON1, BUTTON_MENUOPTIONS_CONTROLS_CUST_UNEQUIPWEAPON2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_previous", BUTTON_MENUOPTIONS_CONTROLS_CUST_PREVIOUS1, BUTTON_MENUOPTIONS_CONTROLS_CUST_PREVIOUS2);
		addControlRow(y, "system_menus_options_input_customize_controls_next", BUTTON_MENUOPTIONS_CONTROLS_CUST_NEXT1, BUTTON_MENUOPTIONS_CONTROLS_CUST_NEXT2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_quickload", BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKLOAD, BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKLOAD2);
		addControlRow(y, "system_menus_options_input_customize_controls_quicksave", BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKSAVE, BUTTON_MENUOPTIONS_CONTROLS_CUST_QUICKSAVE2);
		
		addControlRow(y, "system_menus_options_input_customize_controls_bookmap", BUTTON_MENUOPTIONS_CONTROLS_CUST_MINIMAP1, BUTTON_MENUOPTIONS_CONTROLS_CUST_MINIMAP2, "?", "2");
		
		addControlRow(y, "system_menus_options_input_customize_controls_toggle_fullscreen", BUTTON_MENUOPTIONS_CONTROLS_CUST_TOGGLE_FULLSCREEN1, BUTTON_MENUOPTIONS_CONTROLS_CUST_TOGGLE_FULLSCREEN2, "Toggle fullscreen");
		
		HorizontalPanelWidget * pc = new HorizontalPanelWidget;
		
		{
			ButtonWidget * cb = new ButtonWidget(RATIO_2(Vec2i(20, 380)), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS_INPUT_CUSTOMIZE_KEYS_1;
			cb->SetShortCut(Keyboard::Key_Escape);
			pc->AddElementNoCenterIn(cb);
		}
		
		{
			std::string szMenuText = getLocalised( "system_menus_options_input_customize_default" );
			TextWidget * me = new TextWidget(BUTTON_MENUOPTIONS_CONTROLS_CUST_DEFAULT, hFontMenu, szMenuText);
			me->SetPos(Vec2i((RATIO_X(m_size.x) - me->m_rect.width())*0.5f, RATIO_Y(380)));
			pc->AddElementNoCenterIn(me);
		}
		
		add(pc);
		ReInitActionKey();
	}
};

class QuitConfirmMenuPage : public MenuPage {
public:
	QuitConfirmMenuPage(Vec2i pos, Vec2i size)
		: MenuPage(pos, size, QUIT)
	{}
	
	void init() {
		
		{
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_menus_main_quit"));
			me->SetCheckOff();
			addCenter(me, true);
		}
		
		{
			TextWidget * me = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_menus_main_editquest_confirm"));
			me->SetCheckOff();
			addCenter(me, true);
		}
		
		{
			HorizontalPanelWidget *pPanel = new HorizontalPanelWidget;
			
			TextWidget * yes = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_yes"));
			yes->clicked = boost::bind(ARXMenu_Quit);
			yes->SetPos(Vec2i(RATIO_X(m_size.x-10)-yes->m_rect.width(), 0));
			pPanel->AddElementNoCenterIn(yes);
			
			TextWidget * no = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_no"), Vec2i(RATIO_X(10), 0));
			no->m_targetMenu = MAIN;
			no->SetShortCut(Keyboard::Key_Escape);
			pPanel->AddElementNoCenterIn(no);
			
			pPanel->Move(Vec2i(0, RATIO_Y(380)));
			add(pPanel);
		}
	}
};





void MainMenuLeftCreate(MENUSTATE eMenuState)
{
	mainMenu->eOldMenuState=eMenuState;
	
	delete pWindowMenu, pWindowMenu = NULL;
	
	Vec2i windowMenuPos = Vec2i(20, 25);
	Vec2i windowMenuSize = Vec2i(321, 430);
	
	pWindowMenu = new CWindowMenu(windowMenuPos, windowMenuSize);
	
	Vec2i offset = Vec2i(0, 0);
	Vec2i size = windowMenuSize - offset;
	
	pWindowMenu->m_currentPageId = eMenuState;
	
	{
		NewQuestMenuPage * page = new NewQuestMenuPage(offset, size);
		page->init();
		pWindowMenu->add(page);
	}

	{
	ChooseLoadOrSaveMenuPage * page = new ChooseLoadOrSaveMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	LoadMenuPage * page = new LoadMenuPage(offset + Vec2i(0, -40), size);
	page->m_savegame = SavegameHandle::Invalid;
	page->m_rowSpacing = 5;
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	SaveMenuPage * page = new SaveMenuPage(offset + Vec2i(0, -40), size);
	page->m_rowSpacing = 5;
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	SaveConfirmMenuPage * page = new SaveConfirmMenuPage(offset, size);
	page->m_savegame = SavegameHandle::Invalid;
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	OptionsMenuPage * page = new OptionsMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	VideoOptionsMenuPage * page = new VideoOptionsMenuPage(offset + Vec2i(0, -35), size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	AudioOptionsMenuPage * page = new AudioOptionsMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	InputOptionsMenuPage * page = new InputOptionsMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	ControlOptionsMenuPage1 * page = new ControlOptionsMenuPage1(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	ControlOptionsMenuPage2 * page = new ControlOptionsMenuPage2(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	QuitConfirmMenuPage * page = new QuitConfirmMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
}



MainMenu::MainMenu()
	: bReInitAll(false)
	, eOldMenuState(NOP)
	, eOldMenuWindowState(NOP)
	, m_selected(NULL)
	, m_background(NULL)
	, m_widgets(new WidgetContainer())
	, m_resumeGame(NULL)
{}

MainMenu::~MainMenu() {
	delete m_widgets;
	delete m_background;
}

void MainMenu::init()
{
	m_background = TextureContainer::LoadUI("graph/interface/menus/menu_main_background", TextureContainer::NoColorKey);

	Vec2i pos = Vec2i(370, 100);
	int yOffset = 50;
	
	{
	std::string szMenuText = getLocalised("system_menus_main_resumegame");
	TextWidget *me = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, RATIO_2(pos));
	me->clicked = boost::bind(&MainMenu::onClickedResumeGame, this);
	me->m_targetMenu = RESUME_GAME;
	add(me);
	m_resumeGame = me;
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_newquest");
	TextWidget *me = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, RATIO_2(pos));
	me->clicked = boost::bind(&MainMenu::onClickedNewQuest, this);
	me->m_targetMenu = NEW_QUEST;
	add(me);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_editquest");
	TextWidget *me = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, RATIO_2(pos));
	me->m_targetMenu = EDIT_QUEST;
	add(me);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_options");
	TextWidget *me = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, RATIO_2(pos));
	me->m_targetMenu = OPTIONS;
	add(me);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_credits");
	TextWidget *me = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, RATIO_2(pos));
	me->clicked = boost::bind(ARXMenu_Credits);
	me->m_targetMenu = CREDITS;
	add(me);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_quit");
	TextWidget *me = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, RATIO_2(pos));
	me->m_targetMenu = QUIT;
	add(me);
	}
	pos.y += yOffset;
	
	std::string version = arx_version;
	if(!arx_release_codename.empty()) {
		version += " \"";
		version += arx_release_codename;
		version += "\"";
	}

	float verPosX = RATIO_X(620) - hFontControls->getTextSize(version).x;
	TextWidget * me = new TextWidget(BUTTON_INVALID, hFontControls, version, Vec2i(verPosX, RATIO_Y(80)));
	
	me->SetCheckOff();
	me->lColor=Color(127,127,127);
	add(me);
}

void MainMenu::onClickedResumeGame(){
	pTextManage->Clear();
	ARXMenu_ResumeGame();
}

void MainMenu::onClickedNewQuest() {
	if(!ARXMenu_CanResumeGame()) {
		ARXMenu_NewQuest();
	}
}


void MainMenu::add(Widget * widget) {
	m_widgets->add(widget);
}

MENUSTATE MainMenu::Update() {
	
	if(m_resumeGame) {
		if(ARXMenu_CanResumeGame()) {
			m_resumeGame->SetCheckOn();
			m_resumeGame->lColor = Color(232, 204, 142);
		} else {
			m_resumeGame->SetCheckOff();
			m_resumeGame->lColor = Color(127, 127, 127);
		}
	}
	
	m_selected = m_widgets->getAtPos(GInput->getMousePosAbs());
	
	if(m_selected && GInput->getMouseButton(Mouse::Button_0)) {
		m_selected->OnMouseClick();
		return m_selected->m_targetMenu;
	}
	
	return NOP;
}

// TODO remove this
extern bool bNoMenu;
extern float ARXDiffTimeMenu;

void MainMenu::Render() {

	if(bNoMenu)
		return;

	if(m_background)
		EERIEDrawBitmap2(Rectf(Vec2f(0, 0), g_size.width(), g_size.height()), 0.999f, m_background, Color::white);
	
	int iARXDiffTimeMenu = checked_range_cast<int>(ARXDiffTimeMenu);

	for(size_t i = 0; i < m_widgets->GetNbZone(); ++i) {
		Widget * widget = m_widgets->GetZoneNum(i);
		widget->Update(iARXDiffTimeMenu);
		widget->Render();
	}

	//HIGHLIGHT
	if(m_selected) {
		m_selected->RenderMouseOver();
	}

	//DEBUG ZONE
	GRenderer->ResetTexture(0);
	m_widgets->drawDebug();
}

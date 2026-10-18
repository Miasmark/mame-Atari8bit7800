// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    ui/filemngr.cpp

    MESS's clunky built-in file manager

    TODO
        - Restrict directory listing by file extension

*********************************************************************/

#include "emu.h"
#include "ui/filemngr.h"

#include "ui/filesel.h"
#include "ui/floppycntrl.h"
#include "ui/imgcntrl.h"
#include "ui/prscntrl.h"
#include "ui/miscmenu.h"
#include "ui/ui.h"

#include "softlist.h"

#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <utility>


namespace ui {

/***************************************************************************
    FILE MANAGER
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_file_manager::menu_file_manager(mame_ui_manager &mui, render_target &target, std::string &&warnings)
	: menu(mui, target)
	, m_warnings(std::move(warnings))
	, m_selected_device(nullptr)
	, m_footer_space(3)
{
	// The warning string is used when accessing from the force_file_manager call, i.e.
	// when the file manager is loaded top front in the case of mandatory image devices
	set_heading(_("Media Control"));
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

menu_file_manager::~menu_file_manager()
{
}


//-------------------------------------------------
//  recompute_metrics - recompute metrics
//-------------------------------------------------

void menu_file_manager::recompute_metrics(uint32_t width, uint32_t height, float aspect)
{
	menu::recompute_metrics(width, height, aspect);

	if (!m_warnings.empty())
	{
		m_warnings_layout.reset();

		float const max_width(1.0F - (4.0F * lr_border()));
		m_warnings_layout.emplace(create_layout(max_width, text_layout::text_justify::LEFT));
		m_warnings_layout->add_text(m_warnings, ui().colors().text_color());
	}

	update_footer();
}


//-------------------------------------------------
//  footer_lines - what is shown under the menu
//  for an image: its file or software, and what
//  the media is
//-------------------------------------------------

std::vector<std::string> menu_file_manager::footer_lines(device_image_interface &img)
{
	std::vector<std::string> lines;
	if (!img.exists())
		return lines;

	if (img.has_preset_images_selection())
	{
		lines.emplace_back(util::string_format(_("%1$s preset %2$u"), img.filename(), img.current_preset_image_id() + 1));
	}
	else if (!img.loaded_through_softlist())
	{
		lines.emplace_back(img.filename());
	}
	else
	{
		software_info const &swinfo(*img.software_entry());
		lines.emplace_back(swinfo.longname());
		lines.emplace_back(util::string_format(_("%1$s, %2$s"), swinfo.publisher(), swinfo.year()));
		switch (swinfo.supported())
		{
		case software_support::UNSUPPORTED:
			lines.emplace_back(_("Not supported"));
			break;
		case software_support::PARTIALLY_SUPPORTED:
			lines.emplace_back(_("Partially supported"));
			break;
		case software_support::SUPPORTED:
			break;
		}
	}

	// a line at a time
	std::string const details(img.media_details());
	std::string_view rest(details);
	while (!rest.empty())
	{
		auto const end(std::min(rest.find('\n'), rest.size()));
		lines.emplace_back(rest.substr(0, end));
		rest.remove_prefix(std::min(end + 1, rest.size()));
	}
	return lines;
}


//-------------------------------------------------
//  update_footer - take the footer of the selected
//  image, and keep the space for the biggest one
//-------------------------------------------------

void menu_file_manager::update_footer()
{
	m_footer = m_selected_device ? footer_lines(*m_selected_device) : std::vector<std::string>();

	unsigned space = 3;
	for (device_image_interface &img : image_interface_enumerator(machine().root_device()))
	{
		if (img.has_preset_images_selection() || img.user_loadable())
			space = std::max<unsigned>(space, footer_lines(img).size());
	}
	m_footer_space = space;

	float const footer((float(space) * line_height()) + (3.0F * tb_border()));
	if (m_warnings_layout)
		set_custom_space(0.0F, footer + (float(m_warnings_layout->lines()) * line_height()) + (3.0F * tb_border()));
	else
		set_custom_space(0.0F, footer);
}


//-------------------------------------------------
//  custom_render - perform our special rendering
//-------------------------------------------------

void menu_file_manager::custom_render(uint32_t flags, void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	// the path or software, and what the media is: wider than the menu if need be, up to the
	// width of the screen, and shrunk to fit past that
	if (!m_footer.empty())
	{
		float width(0.0F);
		for (std::string const &line : m_footer)
			width = std::max(width, get_string_width(line));
		width = std::min(width, 1.0F - (4.0F * lr_border()));
		float const x1(std::min(origx1, 0.5F - (width * 0.5F) - lr_border()));
		float const x2(std::max(origx2, 0.5F + (width * 0.5F) + lr_border()));
		draw_text_box(
				m_footer.begin(), m_footer.end(),
				x1, x2, origy2 + tb_border(), origy2 + (m_footer.size() * line_height()) + (3.0F * tb_border()),
				text_layout::text_justify::CENTER, text_layout::word_wrapping::NEVER, true,
				ui().colors().text_color(), ui().colors().background_color());
	}

	// show the warnings if any, below the space of the footer
	if (m_warnings_layout)
	{
		float const warnings_top(origy2 + (float(m_footer_space) * line_height()) + (4.0F * tb_border()));
		ui().draw_outlined_box(
				container(),
				((1.0F + m_warnings_layout->actual_width()) * 0.5F) + lr_border(), warnings_top,
				((1.0F - m_warnings_layout->actual_width()) * 0.5F) - lr_border(), origy2 + bottom,
				ui().colors().background_color());
		m_warnings_layout->emit(
				container(),
				(1.0F - m_warnings_layout->actual_width()) * 0.5F,
				warnings_top + tb_border());
	}

}


void menu_file_manager::fill_image_line(device_image_interface &img, std::string &instance, std::string &filename)
{
	// get the image type/id
	instance = string_format("%s (%s)", img.instance_name(), img.brief_instance_name());

	if (!img.basename())
	{
		filename = "---";
	}
	else if (!img.loaded_through_softlist())
	{
		filename = img.basename();
	}
	else
	{
		// if the image has been loaded through softlist, also show the loaded part
		// also check if this part has a specific part_id (e.g. "Map Disc", "Bonus Disc", etc.)
		software_part const *const part = img.part_entry();
		auto const partid = img.get_feature("part_id");
		if (partid)
			filename = string_format(_("%1$s:%2$s (%3$s)"), img.basename(), part->name(), partid);
		else
			filename = string_format(_("%1$s:%2$s"), img.basename(), part->name());
	}
}

//-------------------------------------------------
//  populate
//-------------------------------------------------

void menu_file_manager::populate()
{
	m_notifiers.clear();

	// cycle through all devices for this system
	bool missing_mandatory = false;
	std::string tmp_inst, tmp_name;
	device_t *prev_parent = nullptr;
	for (device_t &dev : device_enumerator(machine().root_device()))
	{
		// check whether it owns an image interface
		for (device_image_interface &scan : image_interface_enumerator(dev, 1))
		{
			if ((&scan.device() == &dev) || (!scan.has_preset_images_selection() && !scan.user_loadable()))
				continue;

			// check whether we already had some devices with the same owner: if not, output the owner tag!
			if (&dev != prev_parent)
			{
				item_append(string_format(_("[root%1$s]"), dev.tag()), FLAG_UI_HEADING | FLAG_DISABLE, nullptr);
				prev_parent = &dev;
			}

			if (scan.has_preset_images_selection())
			{
				int const index = item_append(scan.image_type_name(), scan.preset_images_list()[scan.current_preset_image_id()], 0, (void *)&scan);
				m_notifiers.emplace_back(scan.add_media_change_notifier(
						[this, index, &scan] (device_image_interface::media_change_event ev)
						{
							item(index).set_subtext(scan.preset_images_list()[scan.current_preset_image_id()]);
							update_footer();
						}));
			}
			else if (scan.user_loadable())
			{
				if (!scan.basename() && scan.must_be_loaded())
					missing_mandatory = true;

				// finally, append the image interface to the menu
				fill_image_line(scan, tmp_inst, tmp_name);
				int const index = item_append(std::move(tmp_inst), std::move(tmp_name), 0, (void *)&scan);
				m_notifiers.emplace_back(scan.add_media_change_notifier(
						[this, index, &scan] (device_image_interface::media_change_event ev)
						{
							std::string text, subtext;
							fill_image_line(scan, text, subtext);
							item(index).set_subtext(std::move(subtext));
							update_footer();
						}));
			}
		}
	}
	item_append(menu_item_type::SEPARATOR);

	if (m_warnings.empty() || !missing_mandatory)
		item_append(m_warnings.empty() ? _("Reset System") : _("Start System"), 0, (void *)1);
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

bool menu_file_manager::handle(event const *ev)
{
	bool result = false;

	if (ev)
	{
		if (uintptr_t(ev->itemref) == 1)
		{
			if (m_selected_device)
			{
				m_selected_device = nullptr;
				update_footer();
				result = true;
			}

			if (IPT_UI_SELECT == ev->iptkey)
				machine().schedule_hard_reset();
		}
		else
		{
			if (ev->itemref != m_selected_device)
			{
				m_selected_device = (device_image_interface *)ev->itemref;
				update_footer();
				result = true;
			}

			if (m_selected_device && (IPT_UI_SELECT == ev->iptkey))
			{
				if (m_selected_device->has_preset_images_selection())
				{
					menu::stack_push<menu_control_device_preset>(ui(), target(), *m_selected_device);
				}
				else
				{
					floppy_image_device *floppy_device = dynamic_cast<floppy_image_device *>(m_selected_device);
					if (floppy_device)
						menu::stack_push<menu_control_floppy_image>(ui(), target(), *floppy_device);
					else
						menu::stack_push<menu_control_device_image>(ui(), target(), *m_selected_device);
				}
			}
		}
	}
	else
	{
		auto const selected = get_selection_ref();
		if (selected && (uintptr_t(selected) != 1) && (selected != m_selected_device))
		{
			m_selected_device = (device_image_interface *)selected;
			update_footer();
			result = true;
		}
	}

	return result;
}

// force file manager menu
void menu_file_manager::force_file_manager(mame_ui_manager &mui, render_target &target, std::string &&warnings)
{
	// drop any existing menus and start the file manager
	menu::stack_reset(mui);
	menu::stack_push_special_main<menu_file_manager>(mui, target, std::move(warnings));
	mui.show_menu(target);

	// make sure MAME is paused
	mui.machine().pause();
}

} // namespace ui

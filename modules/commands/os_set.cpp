/* OperServ core functions
 *
 * (C) 2003-2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include "module.h"

class CommandOSSet : public Command
{
 private:
	void DoList(CommandSource &source)
	{
		Log(LOG_ADMIN, source, this) << "LIST";

		Anope::string index;

		index = Anope::ReadOnly ? _("%s is enabled") : _("%s is disabled");
		source.Reply(index.c_str(), "READONLY");
		index = Anope::Debug ? _("%s is enabled") : _("%s is disabled");
		source.Reply(index.c_str(), "DEBUG");
		index = Anope::NoExpire ? _("%s is enabled") : _("%s is disabled");
		source.Reply(index.c_str(), "NOEXPIRE");

		return;
	}

	void DoSetReadOnly(CommandSource &source, const std::vector<Anope::string> &params)
	{
		const Anope::string &setting = params.size() > 1 ? params[1] : "";

		if (setting.empty())
		{
			this->OnSyntaxError(source, "READONLY");
			return;
		}

		if (setting.equals_ci("ON"))
		{
			Anope::ReadOnly = true;
			Log(LOG_ADMIN, source, this) << "READONLY ON";
			source.Reply(_("Services are now in \002read-only\002 mode."));
		}
		else if (setting.equals_ci("OFF"))
		{
			Anope::ReadOnly = false;
			Log(LOG_ADMIN, source, this) << "READONLY OFF";
			source.Reply(_("Services are now in \002read-write\002 mode."));
		}
		else
			source.Reply(_("Setting for READONLY must be \002ON\002 or \002OFF\002."));
	}

	void DoSetSuperAdmin(CommandSource &source, const std::vector<Anope::string> &params)
	{
		const Anope::string &setting = params.size() > 1 ? params[1] : "";

		if (!source.GetUser())
			return;

		if (setting.empty())
		{
			this->OnSyntaxError(source, "SUPERADMIN");
			return;
		}

		/**
		 * Allow the user to turn super admin on/off
		 *
		 * Rob
		 **/
		bool super_admin = Config->GetModule(this->owner)->Get<bool>("superadmin");
		if (!super_admin)
			source.Reply(_("Super admin can not be set because it is not enabled in the configuration."));
		else if (setting.equals_ci("ON"))
		{
			source.GetUser()->super_admin = true;
			source.Reply(_("You are now a super admin."));
			Log(LOG_ADMIN, source, this) << "SUPERADMIN ON";
		}
		else if (setting.equals_ci("OFF"))
		{
			source.GetUser()->super_admin = false;
			source.Reply(_("You are no longer a super admin."));
			Log(LOG_ADMIN, source, this) << "SUPERADMIN OFF";
		}
		else
			source.Reply(_("Setting for super admin must be \002ON\002 or \002OFF\002."));
	}

	void DoSetDebug(CommandSource &source, const std::vector<Anope::string> &params)
	{
		const Anope::string &setting = params.size() > 1 ? params[1] : "";

		if (setting.empty())
		{
			this->OnSyntaxError(source, "DEBUG");
			return;
		}

		if (setting.equals_ci("ON"))
		{
			Anope::Debug = 1;
			Log(LOG_ADMIN, source, this) << "DEBUG ON";
			source.Reply(_("Services are now in \002debug\002 mode."));
		}
		else if (setting.equals_ci("OFF") || setting == "0")
		{
			Log(LOG_ADMIN, source, this) << "DEBUG OFF";
			Anope::Debug = 0;
			source.Reply(_("Services are now in \002non-debug\002 mode."));
		}
		else
		{
			try
			{
				Anope::Debug = convertTo<int>(setting);
				Log(LOG_ADMIN, source, this) << "DEBUG " << Anope::Debug;
				source.Reply(_("Services are now in \002debug\002 mode (level %d)."), Anope::Debug);
				return;
			}
			catch (const ConvertException &) { }

			source.Reply(_("Setting for DEBUG must be \002ON\002, \002OFF\002, or a positive number."));
		}

		return;
	}

	void DoSetNoExpire(CommandSource &source, const std::vector<Anope::string> &params)
	{
		const Anope::string &setting = params.size() > 1 ? params[1] : "";

		if (setting.empty())
		{
			this->OnSyntaxError(source, "NOEXPIRE");
			return;
		}

		if (setting.equals_ci("ON"))
		{
			Anope::NoExpire = true;
			Log(LOG_ADMIN, source, this) << "NOEXPIRE ON";
			source.Reply(_("Services are now in \002no expire\002 mode."));
		}
		else if (setting.equals_ci("OFF"))
		{
			Anope::NoExpire = false;
			Log(LOG_ADMIN, source, this) << "NOEXPIRE OFF";
			source.Reply(_("Services are now in \002expire\002 mode."));
		}
		else
			source.Reply(_("Setting for NOEXPIRE must be \002ON\002 or \002OFF\002."));
	}
 public:
	CommandOSSet(Module *creator) : Command(creator, "operserv/set", 1, 2)
	{
		this->SetDesc(_("Set various global Services options"));
		this->SetSyntax(_("\037option\037 \037setting\037"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		const Anope::string &option = params[0];

		if (option.equals_ci("LIST"))
			return this->DoList(source);
		else if (option.equals_ci("READONLY"))
			return this->DoSetReadOnly(source, params);
		else if (option.equals_ci("DEBUG"))
			return this->DoSetDebug(source, params);
		else if (option.equals_ci("NOEXPIRE"))
			return this->DoSetNoExpire(source, params);
		else if (option.equals_ci("SUPERADMIN"))
			return this->DoSetSuperAdmin(source, params);
		else
			this->OnSyntaxError(source, "");
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		if (subcommand.empty())
		{
			source.Reply(_("Sets various global Services options.  Option names currently defined are:\n"
					"    READONLY   Set read-only or read-write mode\n"
					"    DEBUG      Activate or deactivate debug mode\n"
					"    NOEXPIRE   Activate or deactivate no expire mode\n"
					"    SUPERADMIN Activate or deactivate super admin mode\n"
					"    LIST       List the options"));
		}
		else if (subcommand.equals_ci("LIST"))
			//source.Reply(_("Syntax: \002LIST\002\n"
			//		" \n"
			source.Reply(("Display the various {0} settings."), source.service->nick);
		else if (subcommand.equals_ci("READONLY"))
			//source.Reply(_("Syntax: \002READONLY {ON | OFF}\002\n"
			//		" \n"
			source.Reply(_("Sets read-only mode on or off.  In read-only mode, norma users will not be allowed to modify any Services data, including channel and nickname access lists, etc."
				       "Services Operators will still be able to do most tasks, but should understand any changes they do may not be permanent.\n"
				       "\n"
			               "This option is equivalent to the command-line option \002--readonly\002."));
		else if (subcommand.equals_ci("DEBUG"))
			//source.Reply(_("Syntax: \002DEBUG {ON | OFF}\002\n"
			//		" \n"
			source.Reply(_("Sets debug mode on or off.\n"
			               "\n"
			               "This option is equivalent to the command-line option \002--debug\002."));
		else if (subcommand.equals_ci("NOEXPIRE"))
			//source.Reply(_("Syntax: \002NOEXPIRE {ON | OFF}\002\n"
			//		" \n"
			source.Reply(_("Sets no expire mode on or off. In no expire mode, nicks, channels, akills and exceptions won't expire until the option is unset.\n"
			               "\n"
			               "This option is equivalent to the command-line option \002--noexpire\002."));
		else if (subcommand.equals_ci("SUPERADMIN"))
			//source.Reply(_("Syntax: \002SUPERADMIN {ON | OFF}\002\n"
			//		" \n"
			source.Reply(_("Setting this will grant you extra privileges, such as the ability to be \"founder\" on all channels."
			               "\n"
			               "This option is \002not\002 persistent, and should only be used when needed, and set back to OFF when no longer needed."));
		else
			return false;
		
		return true;
	}
};

class OSSet : public Module
{
	CommandOSSet commandosset;

 public:
	OSSet(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, VENDOR)
		, commandosset(this)
	{
	}
};

MODULE_INIT(OSSet)

/* BotServ core functions
 *
 * (C) 2003-2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 *
 */

#include "module.h"
#include "modules/bs_kick.h"
#include "modules/bs_badwords.h"
#include "modules/bs_info.h"

static Module *me;

struct KickerDataImpl : KickerData
{
	KickerDataImpl(Extensible *obj)
	{
		amsgs = badwords = bolds = caps = colors = flood = italics = repeat = reverses = underlines = false;
		for (int16_t i = 0; i < TTB_SIZE; ++i)
			ttb[i] = 0;
		capsmin = capspercent = 0;
		floodlines = floodsecs = 0;
		repeattimes = 0;

		dontkickops = dontkickvoices = false;
	}

	void Check(ChanServ::Channel *ci) override
	{
		if (amsgs || badwords || bolds || caps || colors || flood || italics || repeat || reverses || underlines)
			return;

		ci->Shrink<KickerData>("kickerdata");
	}

	struct ExtensibleItem : ::ExtensibleItem<KickerDataImpl>
	{
		ExtensibleItem(Module *m, const Anope::string &ename) : ::ExtensibleItem<KickerDataImpl>(m, ename) { }

		void ExtensibleSerialize(const Extensible *e, const Serializable *s, Serialize::Data &data) const override
		{
			if (s->GetSerializableType()->GetName() != "ChanServ::Channel")
				return;

			const ChanServ::Channel *ci = anope_dynamic_static_cast<const ChanServ::Channel *>(e);
			KickerData *kd = this->Get(ci);
			if (kd == NULL)
				return;

			data["kickerdata:amsgs"] << kd->amsgs;
			data["kickerdata:badwords"] << kd->badwords;
			data["kickerdata:bolds"] << kd->bolds;
			data["kickerdata:caps"] << kd->caps;
			data["kickerdata:colors"] << kd->colors;
			data["kickerdata:flood"] << kd->flood;
			data["kickerdata:italics"] << kd->italics;
			data["kickerdata:repeat"] << kd->repeat;
			data["kickerdata:reverses"] << kd->reverses;
			data["kickerdata:underlines"] << kd->underlines;

			data.SetType("capsmin", Serialize::Data::DT_INT); data["capsmin"] << kd->capsmin;
			data.SetType("capspercent", Serialize::Data::DT_INT); data["capspercent"] << kd->capspercent;
			data.SetType("floodlines", Serialize::Data::DT_INT); data["floodlines"] << kd->floodlines;
			data.SetType("floodsecs", Serialize::Data::DT_INT); data["floodsecs"] << kd->floodsecs;
			data.SetType("repeattimes", Serialize::Data::DT_INT); data["repeattimes"] << kd->repeattimes;
			for (int16_t i = 0; i < TTB_SIZE; ++i)
				data["ttb"] << kd->ttb[i] << " ";
		}

		void ExtensibleUnserialize(Extensible *e, Serializable *s, Serialize::Data &data) override
		{
			if (s->GetSerializableType()->GetName() != "ChanServ::Channel")
				return;

			ChanServ::Channel *ci = anope_dynamic_static_cast<ChanServ::Channel *>(e);
			KickerData *kd = ci->Require<KickerData>("kickerdata");

			data["kickerdata:amsgs"] >> kd->amsgs;
			data["kickerdata:badwords"] >> kd->badwords;
			data["kickerdata:bolds"] >> kd->bolds;
			data["kickerdata:caps"] >> kd->caps;
			data["kickerdata:colors"] >> kd->colors;
			data["kickerdata:flood"] >> kd->flood;
			data["kickerdata:italics"] >> kd->italics;
			data["kickerdata:repeat"] >> kd->repeat;
			data["kickerdata:reverses"] >> kd->reverses;
			data["kickerdata:underlines"] >> kd->underlines;

			data["capsmin"] >> kd->capsmin;
			data["capspercent"] >> kd->capspercent;
			data["floodlines"] >> kd->floodlines;
			data["floodsecs"] >> kd->floodsecs;
			data["repeattimes"] >> kd->repeattimes;

			Anope::string ttb, tok;
			data["ttb"] >> ttb;
			spacesepstream sep(ttb);
			for (int i = 0; sep.GetToken(tok) && i < TTB_SIZE; ++i)
				try
				{
					kd->ttb[i] = convertTo<int16_t>(tok);
				}
				catch (const ConvertException &) { }

			kd->Check(ci);
		}
	};
};

class CommandBSKick : public Command
{
 public:
	CommandBSKick(Module *creator) : Command(creator, "botserv/kick", 0)
	{
		this->SetDesc(_("Configures kickers"));
		this->SetSyntax(_("\037option\037 \037channel\037 {\037ON|OFF\037} [\037settings\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		this->OnSyntaxError(source, "");
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Configures bot kickers."
		               " Use of this command requires the \002SET\002 privilege on \037channel\037.\n"
		               "\n"
		               "Available kickers:"));

		Anope::string this_name = source.command;
		for (CommandInfo::map::const_iterator it = source.service->commands.begin(), it_end = source.service->commands.end(); it != it_end; ++it)
		{
			const Anope::string &c_name = it->first;
			const CommandInfo &info = it->second;

			if (c_name.find_ci(this_name + " ") == 0)
			{
				ServiceReference<Command> command("Command", info.name);
				if (command)
				{
					source.command = c_name;
					command->OnServHelp(source);
				}
			}
		}

		CommandInfo *help = source.service->FindCommand("generic/help");
		if (help)
			source.Reply(_("See \002{0}{1} {2} {3} \037option\037\002 for more information on a specific option."),
		                       Config->StrictPrivmsg, source.service->nick, help->cname, this_name);

		return true;
	}
};

class CommandBSKickBase : public Command
{
 public:
	CommandBSKickBase(Module *creator, const Anope::string &cname, int minarg, int maxarg) : Command(creator, cname, minarg, maxarg)
	{
	}

	virtual void Execute(CommandSource &source, const std::vector<Anope::string> &params) override anope_abstract;

	virtual bool OnHelp(CommandSource &source, const Anope::string &subcommand) override anope_abstract;

 protected:
	bool CheckArguments(CommandSource &source, const std::vector<Anope::string> &params, ChanServ::Channel* &ci)
	{
		const Anope::string &chan = params[0];
		const Anope::string &option = params[1];

		ci = ChanServ::Find(chan);

		if (Anope::ReadOnly)
			source.Reply(_("Sorry, kicker configuration is temporarily disabled."));
		else if (ci == NULL)
			source.Reply(_("Channel \002{0}\002 isn't registered."), chan);
		else if (option.empty())
			this->OnSyntaxError(source, "");
		else if (!option.equals_ci("ON") && !option.equals_ci("OFF"))
			this->OnSyntaxError(source, "");
		else if (!source.AccessFor(ci).HasPriv("SET") && !source.HasPriv("botserv/administration"))
			source.Reply(_("Access denied. You do not have privilege \002{0}\002 on \002{1}\002."), "SET", ci->name);
		else if (!ci->bi)
			source.Reply(_("There is no bot assigned to \002{0}\002."), ci->name);
		else
			return true;

		return false;
	}

	bool CheckTTB(CommandSource &source, const Anope::string &ttb, int16_t &i)
	{
		i = 0;
		if (ttb.empty())
			return true;

		try
		{
			i = convertTo<int16_t>(ttb);
			if (i < 0)
				throw ConvertException();
		}
		catch (const ConvertException &)
		{
			i = 0;
			source.Reply(_("\002{0}\002 can not be taken as times to ban. Times to ban must be a positive integer."), ttb);
			return false;
		}

		return true;
	}

	void Process(CommandSource &source, ChanServ::Channel *ci, const Anope::string &param, const Anope::string &ttb, size_t ttb_idx, const Anope::string &optname, KickerData *kd, bool &val)
	{
		if (param.equals_ci("ON"))
		{
			int16_t i;
			if (!CheckTTB(source, ttb, i))
				return;

			kd->ttb[ttb_idx] = i;

			val = true;
			if (kd->ttb[ttb_idx])
				source.Reply(_("Bot will now kick for \002{0}\002, and will place a ban after \002{1}\002 kicks for the same user."), optname, kd->ttb[ttb_idx]);
			else
				source.Reply(_("Bot will now kick for \002{0}\002."), optname);

			bool override = !source.AccessFor(ci).HasPriv("SET");
			Log(override ? LOG_OVERRIDE : LOG_COMMAND, source, this, ci) << "to enable the " << optname << "kicker";
		}
		else if (param.equals_ci("OFF"))
		{
			bool override = !source.AccessFor(ci).HasPriv("SET");
			Log(override ? LOG_OVERRIDE : LOG_COMMAND, source, this, ci) << "to disable the " << optname << "kicker";

			val = false;
			source.Reply(_("Bot won't kick for \002{0}\002 anymore."), optname);
		}
		else
			this->OnSyntaxError(source, "");
	}
};

class CommandBSKickAMSG : public CommandBSKickBase
{
 public:
	CommandBSKickAMSG(Module *creator) : CommandBSKickBase(creator, "botserv/kick/amsg", 2, 3)
	{
		this->SetDesc(_("Configures AMSG kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (CheckArguments(source, params, ci))
		{
			KickerData *kd = ci->Require<KickerData>("kickerdata");
			Process(source, ci, params[1], params.size() > 2 ? params[2] : "", TTB_AMSGS, "AMSG", kd, kd->amsgs);
			kd->Check(ci);
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the AMSG kicker on or off on \037channel\037. When enabled, the bot will kick users who send the same message to multiple channels where {0} bots are.\n"
		               "\n"
		               "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."),
		               source.service->nick);
		return true;
	}
};

class CommandBSKickBadwords : public CommandBSKickBase
{
 public:
	CommandBSKickBadwords(Module *creator) : CommandBSKickBase(creator, "botserv/kick/badwords", 2, 3)
	{
		this->SetDesc(_("Configures badwords kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (CheckArguments(source, params, ci))
		{
			KickerData *kd = ci->Require<KickerData>("kickerdata");
			Process(source, ci, params[1], params.size() > 2 ? params[2] : "", TTB_BADWORDS, "badwords", kd, kd->badwords);
			kd->Check(ci);
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		BotInfo *bi;
		Anope::string name;
		CommandInfo *help;
		source.Reply(_("Sets the bad words kicker on or off on \037channel\037. When enabled, the bot will kick users who say certain words on the channel."));
		if (Command::FindCommandFromService("botserv/badwords", bi, name) && (help = bi->FindCommand("generic/help")))
			source.Reply(_("You can define bad words for your channel using the \002{0}\002 command. See \002{1}{2} {3} {4}\002 for more information."),
			               name, Config->StrictPrivmsg, bi->nick, help->cname, name);
		source.Reply(_("\n"
		               "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickBolds : public CommandBSKickBase
{
 public:
	CommandBSKickBolds(Module *creator) : CommandBSKickBase(creator, "botserv/kick/bolds", 2, 3)
	{
		this->SetDesc(_("Configures badwords kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (CheckArguments(source, params, ci))
		{
			KickerData *kd = ci->Require<KickerData>("kickerdata");
			Process(source, ci, params[1], params.size() > 2 ? params[2] : "", TTB_BOLDS, "bolds", kd, kd->bolds);
			kd->Check(ci);
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the bolds kicker on or off on \037channel\037. When enabled, the bot will kick users who use \002bolds\002.\n"
		               "\n"
		               "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickCaps : public CommandBSKickBase
{
 public:
	CommandBSKickCaps(Module *creator) : CommandBSKickBase(creator, "botserv/kick/caps", 2, 5)
	{
		this->SetDesc(_("Configures caps kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037 [\037min\037 [\037percent\037]]]\002"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (!CheckArguments(source, params, ci))
			return;

		KickerData *kd = ci->Require<KickerData>("kickerdata");

		if (params[1].equals_ci("ON"))
		{
			const Anope::string &ttb = params.size() > 2 ? params[2] : "",
				&min = params.size() > 3 ? params[3] : "",
				&percent = params.size() > 4 ? params[4] : "";

			int16_t i;
			if (!CheckTTB(source, ttb, i))
				return;

			kd->ttb[TTB_CAPS] = i;
			kd->capsmin = 10;
			try
			{
				kd->capsmin = convertTo<int16_t>(min);
			}
			catch (const ConvertException &) { }
			if (kd->capsmin < 1)
				kd->capsmin = 10;

			kd->capspercent = 25;
			try
			{
				kd->capspercent = convertTo<int16_t>(percent);
			}
			catch (const ConvertException &) { }
			if (kd->capspercent < 1 || kd->capspercent > 100)
				kd->capspercent = 25;

			kd->caps = true;
			if (kd->ttb[TTB_CAPS])
				source.Reply(_("Bot will now kick for \002caps\002 if they constitute at least {0} characters and {1}% of the entire message, and will place a ban after {2} kicks for the same user."), kd->capsmin, kd->capspercent, kd->ttb[TTB_CAPS]);
			else
				source.Reply(_("Bot will now kick for \002caps\002 if they constitute at least {0} characters and {1}% of the entire message."), kd->capsmin, kd->capspercent);
		}
		else
		{
			kd->caps = false;
			source.Reply(_("Bot won't kick for \002caps\002 anymore."));
		}

		kd->Check(ci);
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the caps kicker on or off on \037channel\037. When enabled, the bot will kick users who talk in CAPS."
		                " The bot kicks only if there are at least \002min\002 caps and they constitute at least \002percent\002% of the total text line."
		                " (if not given, it defaults to 10 characters and 25%).\n"
		                "\n"
		                "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickColors : public CommandBSKickBase
{
 public:
	CommandBSKickColors(Module *creator) : CommandBSKickBase(creator, "botserv/kick/colors", 2, 3)
	{
		this->SetDesc(_("Configures color kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (CheckArguments(source, params, ci))
		{
			KickerData *kd = ci->Require<KickerData>("kickerdata");
			Process(source, ci, params[1], params.size() > 2 ? params[2] : "", TTB_COLORS, "colors", kd, kd->colors);
			kd->Check(ci);
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the colors kicker on or off on \037channel\037. When enabled, the bot will kick users who use \00304c\00307o\00308l\00303o\00306r\00312s\017.\n"
		               "\n"
		                "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickFlood : public CommandBSKickBase
{
 public:
	CommandBSKickFlood(Module *creator) : CommandBSKickBase(creator, "botserv/kick/flood", 2, 5)
	{
		this->SetDesc(_("Configures flood kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037 [\037lines\037 [\037seconds\037]]]\002"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (!CheckArguments(source, params, ci))
			return;

		KickerData *kd = ci->Require<KickerData>("kickerdata");

		if (params[1].equals_ci("ON"))
		{
			const Anope::string &ttb = params.size() > 2 ? params[2] : "",
						&lines = params.size() > 3 ? params[3] : "",
						&secs = params.size() > 4 ? params[4] : "";

			int16_t i;
			if (!CheckTTB(source, ttb, i))
				return;

			kd->ttb[TTB_FLOOD] = i;
			kd->floodlines = 6;
			try
			{
				kd->floodlines = convertTo<int16_t>(lines);
			}
			catch (const ConvertException &) { }
			if (kd->floodlines < 2)
				kd->floodlines = 6;

			kd->floodsecs = 10;
			try
			{
				kd->floodsecs = convertTo<int16_t>(secs);
			}
			catch (const ConvertException &) { }
			if (kd->floodsecs < 1)
				kd->floodsecs = 10;
			if (kd->floodsecs > Config->GetModule(me)->Get<time_t>("keepdata"))
				kd->floodsecs = Config->GetModule(me)->Get<time_t>("keepdata");

			kd->flood = true;
			if (kd->ttb[TTB_FLOOD])
				source.Reply(_("Bot will now kick for \002flood\002 ({0} lines in {1} seconds and will place a ban after {2} kicks for the same user."), kd->floodlines, kd->floodsecs, kd->ttb[TTB_FLOOD]);
			else
				source.Reply(_("Bot will now kick for \002flood\002 ({0} lines in {1} seconds)."), kd->floodlines, kd->floodsecs);
		}
		else if (params[1].equals_ci("OFF"))
		{
			kd->flood = false;
			source.Reply(_("Bot won't kick for \002flood\002 anymore."));
		}
		else
			this->OnSyntaxError(source, params[1]);

		kd->Check(ci);
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the flood kicker on or off on \037channel\037. When enabled, the bot to kick users who are flooding the channel with at least \037lines\037 lines in \037seconds\037 seconds. If not given, it defaults to 6 lines in 10 seconds.\n"
		               "\n"
		                "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickItalics : public CommandBSKickBase
{
 public:
	CommandBSKickItalics(Module *creator) : CommandBSKickBase(creator, "botserv/kick/italics", 2, 3)
	{
		this->SetDesc(_("Configures italics kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (CheckArguments(source, params, ci))
		{
			KickerData *kd = ci->Require<KickerData>("kickerdata");
			Process(source, ci, params[1], params.size() > 2 ? params[2] : "", TTB_ITALICS, "italics", kd, kd->italics);
			kd->Check(ci);
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the italics kicker on or off on \037channel\037. When enabled, the bot will kick users who use \035italics\035."
		               "\n"
		               "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickRepeat : public CommandBSKickBase
{
 public:
	CommandBSKickRepeat(Module *creator) : CommandBSKickBase(creator, "botserv/kick/repeat", 2, 4)
	{
		this->SetDesc(_("Configures repeat kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037 [\037num\037]]\002"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (!CheckArguments(source, params, ci))
			return;

		KickerData *kd = ci->Require<KickerData>("kickerdata");

		if (params[1].equals_ci("ON"))
		{
			const Anope::string &ttb = params.size() > 2 ? params[2] : "",
						&times = params.size() > 3 ? params[3] : "";

			int16_t i;
			if (!CheckTTB(source, ttb, i))
				return;

			kd->ttb[TTB_REPEAT] = i;
			kd->repeattimes = 3;
			try
			{
				kd->repeattimes = convertTo<int16_t>(times);
			}
			catch (const ConvertException &) { }
			if (kd->repeattimes < 2)
				kd->repeattimes = 3;

			kd->repeat = true;
			if (kd->ttb[TTB_REPEAT])
				source.Reply(_("Bot will now kick for \002repeats\002 (users that say the same thing {0} times), and will place a ban after {1} kicks for the same user."), kd->repeattimes + 1, kd->ttb[TTB_REPEAT]);
			else
				source.Reply(_("Bot will now kick for \002repeats\002 (users that say the same thing {0} times)."), kd->repeattimes + 1);
		}
		else if (params[1].equals_ci("OFF"))
		{
			kd->repeat = false;
			source.Reply(_("Bot won't kick for \002repeats\002 anymore."));
		}
		else
			this->OnSyntaxError(source, params[1]);

		kd->Check(ci);
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the repeat kicker on or off. When enabled, the bot will kick users who repeat themselves \037num\037 times. If \037num\037 is not given, it defaults to 3.\n"
		               "\n"
		               "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickReverses : public CommandBSKickBase
{
 public:
 	CommandBSKickReverses(Module *creator) : CommandBSKickBase(creator, "botserv/kick/reverses", 2, 3)
	{
		this->SetDesc(_("Configures reverses kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (CheckArguments(source, params, ci))
		{
			KickerData *kd = ci->Require<KickerData>("kickerdata");
			Process(source, ci, params[1], params.size() > 2 ? params[2] : "", TTB_REVERSES, "reverses", kd, kd->reverses);
			kd->Check(ci);
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the reverses kicker on or off. When enabled, the bot will kick users who use \026reverses\026.\n"
		                "\n"
		                "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSKickUnderlines : public CommandBSKickBase
{
 public:
	CommandBSKickUnderlines(Module *creator) : CommandBSKickBase(creator, "botserv/kick/underlines", 2, 3)
	{
		this->SetDesc(_("Configures underlines kicker"));
		this->SetSyntax(_("\037channel\037 {\037ON|OFF\037} [\037ttb\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci;
		if (CheckArguments(source, params, ci))
		{
			KickerData *kd = ci->Require<KickerData>("kickerdata");
			Process(source, ci, params[1], params.size() > 2 ? params[2] : "", TTB_UNDERLINES, "underlines", kd, kd->underlines);
			kd->Check(ci);
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Sets the underlines kicker on or off. When enabled, the bot will kick users who use \037underlines\037\n"
		               "\n"
		               "\037ttb\037 is the number of times a user can be kicked before they get banned. Don't give ttb to disable the ban system."));
		return true;
	}
};

class CommandBSSetDontKickOps : public Command
{
 public:
	CommandBSSetDontKickOps(Module *creator, const Anope::string &sname = "botserv/set/dontkickops") : Command(creator, sname, 2, 2)
	{
		this->SetDesc(_("To protect ops against bot kicks"));
		this->SetSyntax(_("\037channel\037 {ON | OFF}"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci = ChanServ::Find(params[0]);
		if (ci == NULL)
		{
			source.Reply(_("Channel \002{0}\002 isn't registered."), params[0]);
			return;
		}

		ChanServ::AccessGroup access = source.AccessFor(ci);
		if (!source.HasPriv("botserv/administration") && !access.HasPriv("SET"))
		{
			source.Reply(_("Access denied. You do not have privilege \002{0}\002 on \002{1}\002."), "SET", ci->name);
			return;
		}

		if (Anope::ReadOnly)
		{
			source.Reply(_("Sorry, bot option setting is temporarily disabled."));
			return;
		}

		KickerData *kd = ci->Require<KickerData>("kickerdata");
		if (params[1].equals_ci("ON"))
		{
			bool override = !access.HasPriv("SET");
			Log(override ? LOG_OVERRIDE : LOG_COMMAND, source, this, ci) << "to enable dontkickops";

			kd->dontkickops = true;
			source.Reply(_("Bot \002won't kick ops\002 on channel \002{0}\002."), ci->name);
		}
		else if (params[1].equals_ci("OFF"))
		{
			bool override = !access.HasPriv("SET");
			Log(override ? LOG_OVERRIDE : LOG_COMMAND, source, this, ci) << "to disable dontkickops";

			kd->dontkickops = false;
			source.Reply(_("Bot \002will kick ops\002 on channel \002{0}\002."), ci->name);
		}
		else
			this->OnSyntaxError(source, source.command);

		kd->Check(ci);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		source.Reply(_("Enables or disables \002op protection\002 mode on a channel."
		               " When it is enabled, ops won't be kicked by the bot, even if they don't have the \002{0}\002 privilege.\n"
		               "\n"
		               "Use of this command requires the \002{1}\002 privilege on \037channel\037."),
		               "NOKICK", "SET");
		return true;
	}
};

class CommandBSSetDontKickVoices : public Command
{
 public:
	CommandBSSetDontKickVoices(Module *creator, const Anope::string &sname = "botserv/set/dontkickvoices") : Command(creator, sname, 2, 2)
	{
		this->SetDesc(_("To protect voices against bot kicks"));
		this->SetSyntax(_("\037channel\037 {ON | OFF}"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChanServ::Channel *ci = ChanServ::Find(params[0]);
		if (ci == NULL)
		{
			source.Reply(_("Channel \002{0}\002 isn't registered."), params[0]);
			return;
		}

		ChanServ::AccessGroup access = source.AccessFor(ci);
		if (!source.HasPriv("botserv/administration") && !access.HasPriv("SET"))
		{
			source.Reply(_("Access denied. You do not have privilege \002{0}\002 on \002{1}\002."), "SET", ci->name);
			return;
		}

		if (Anope::ReadOnly)
		{
			source.Reply(_("Sorry, bot option setting is temporarily disabled."));
			return;
		}

		KickerData *kd = ci->Require<KickerData>("kickerdata");
		if (params[1].equals_ci("ON"))
		{
			bool override = !access.HasPriv("SET");
			Log(override ? LOG_OVERRIDE : LOG_COMMAND, source, this, ci) << "to enable dontkickvoices";

			kd->dontkickvoices = true;
			source.Reply(_("Bot \002won't kick voices\002 on channel %s."), ci->name.c_str());
		}
		else if (params[1].equals_ci("OFF"))
		{
			bool override = !access.HasPriv("SET");
			Log(override ? LOG_OVERRIDE : LOG_COMMAND, source, this, ci) << "to disable dontkickvoices";

			kd->dontkickvoices = false;
			source.Reply(_("Bot \002will kick voices\002 on channel %s."), ci->name.c_str());
		}
		else
			this->OnSyntaxError(source, source.command);

		kd->Check(ci);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		source.Reply(_("Enables or disables \002voice protection\002 mode on a channel."
		               " When it is enabled, ops won't be kicked by the bot, even if they don't have the \002{0}\002 privilege.\n"
		               "\n"
		               "Use of this command requires the \002{1}\002 privilege on \037channel\037."),
		               "NOKICK", "SET");
		return true;
	}
};

struct BanData
{
	struct Data
	{
		Anope::string mask;
		time_t last_use;
		int16_t ttb[TTB_SIZE];

		Data()
		{
			last_use = 0;
			for (int i = 0; i < TTB_SIZE; ++i)
				this->ttb[i] = 0;
		}
	};

 private:
	typedef Anope::map<Data> data_type;
	data_type data_map;

 public:
 	BanData(Extensible *) { }

	Data &get(const Anope::string &key)
	{
		return this->data_map[key];
	}

	bool empty() const
	{
		return this->data_map.empty();
	}

	void purge()
	{
		time_t keepdata = Config->GetModule(me)->Get<time_t>("keepdata");
		for (data_type::iterator it = data_map.begin(), it_end = data_map.end(); it != it_end;)
		{
			const Anope::string &user = it->first;
			Data &bd = it->second;
			++it;

			if (Anope::CurTime - bd.last_use > keepdata)
				data_map.erase(user);
		}
	}
};

struct UserData
{
	UserData(Extensible *)
	{
		last_use = last_start = Anope::CurTime;
		lines = times = 0;
		lastline.clear();
	}

	/* Data validity */
	time_t last_use;

	/* for flood kicker */
	int16_t lines;
	time_t last_start;

	/* for repeat kicker */
	Anope::string lasttarget;
	int16_t times;

	Anope::string lastline;
};

class BanDataPurger : public Timer
{
 public:
	BanDataPurger(Module *o) : Timer(o, 300, Anope::CurTime, true) { }

	void Tick(time_t) override
	{
		Log(LOG_DEBUG) << "bs_main: Running bandata purger";

		for (channel_map::iterator it = ChannelList.begin(), it_end = ChannelList.end(); it != it_end; ++it)
		{
			Channel *c = it->second;

			BanData *bd = c->GetExt<BanData>("bandata");
			if (bd != NULL)
			{
				bd->purge();
				if (bd->empty())
					c->Shrink<BanData>("bandata");
			}
		}
	}
};

class BSKick : public Module
	, public EventHook<Event::BotInfoEvent>
	, public EventHook<Event::Privmsg>
{
	ExtensibleItem<BanData> bandata;
	ExtensibleItem<UserData> userdata;
	KickerDataImpl::ExtensibleItem kickerdata;

	CommandBSKick commandbskick;
	CommandBSKickAMSG commandbskickamsg;
	CommandBSKickBadwords commandbskickbadwords;
	CommandBSKickBolds commandbskickbolds;
	CommandBSKickCaps commandbskickcaps;
	CommandBSKickColors commandbskickcolors;
	CommandBSKickFlood commandbskickflood;
	CommandBSKickItalics commandbskickitalics;
	CommandBSKickRepeat commandbskickrepeat;
	CommandBSKickReverses commandbskickreverse;
	CommandBSKickUnderlines commandbskickunderlines;

	CommandBSSetDontKickOps commandbssetdontkickops;
	CommandBSSetDontKickVoices commandbssetdontkickvoices;

	BanDataPurger purger;

	EventHandlers<Event::BotBan> onbotban;

	BanData::Data &GetBanData(User *u, Channel *c)
	{
		BanData *bd = bandata.Require(c);
		return bd->get(u->GetMask());
	}

	UserData *GetUserData(User *u, Channel *c)
	{
		ChanUserContainer *uc = c->FindUser(u);
		if (uc == NULL)
			return NULL;

		UserData *ud = userdata.Require(uc);
		return ud;
       }

	void check_ban(ChanServ::Channel *ci, User *u, KickerData *kd, int ttbtype)
	{
		/* Don't ban ulines or protected users */
		if (u->IsProtected())
			return;

		BanData::Data &bd = this->GetBanData(u, ci->c);

		++bd.ttb[ttbtype];
		if (kd->ttb[ttbtype] && bd.ttb[ttbtype] >= kd->ttb[ttbtype])
		{
			/* Should not use == here because bd.ttb[ttbtype] could possibly be > kd->ttb[ttbtype]
			 * if the TTB was changed after it was not set (0) before and the user had already been
			 * kicked a few times. Bug #1056 - Adam */

			bd.ttb[ttbtype] = 0;

			Anope::string mask = ci->GetIdealBan(u);

			ci->c->SetMode(NULL, "BAN", mask);
			this->onbotban(&Event::BotBan::OnBotBan, u, ci, mask);
		}
	}

	void bot_kick(ChanServ::Channel *ci, User *u, const char *message, ...)
	{
		va_list args;
		char buf[1024];

		if (!ci || !ci->bi || !ci->c || !u || u->IsProtected() || !ci->c->FindUser(u))
			return;

		Anope::string fmt = Language::Translate(u, message);
		va_start(args, message);
		vsnprintf(buf, sizeof(buf), fmt.c_str(), args);
		va_end(args);

		ci->c->Kick(ci->bi, u, "%s", buf);
	}

 public:
	BSKick(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, VENDOR)
		, EventHook<Event::BotInfoEvent>("OnBotInfoEvent")
		, EventHook<Event::Privmsg>("OnPrivmsg")
		, bandata(this, "bandata")
		, userdata(this, "userdata")
		, kickerdata(this, "kickerdata")

		, commandbskick(this)
		, commandbskickamsg(this)
		, commandbskickbadwords(this)
		, commandbskickbolds(this)
		, commandbskickcaps(this)
		, commandbskickcolors(this)
		, commandbskickflood(this)
		, commandbskickitalics(this)
		, commandbskickrepeat(this)
		, commandbskickreverse(this)
		, commandbskickunderlines(this)

		, commandbssetdontkickops(this)
		, commandbssetdontkickvoices(this)

		, purger(this)

		, onbotban(this, "OnBotBan")
	{
		me = this;

	}

	void OnBotInfo(CommandSource &source, BotInfo *bi, ChanServ::Channel *ci, InfoFormatter &info) override
	{
		if (!ci)
			return;

		Anope::string enabled = Language::Translate(source.nc, _("Enabled"));
		Anope::string disabled = Language::Translate(source.nc, _("Disabled"));
		KickerData *kd = kickerdata.Get(ci);

		if (kd && kd->badwords)
		{
			if (kd->ttb[TTB_BADWORDS])
				info[_("Bad words kicker")] = Anope::printf("%s (%d kick(s) to ban)", enabled.c_str(), kd->ttb[TTB_BADWORDS]);
			else
				info[_("Bad words kicker")] = enabled;
		}
		else
			info[_("Bad words kicker")] = disabled;

		if (kd && kd->bolds)
		{
			if (kd->ttb[TTB_BOLDS])
				info[_("Bolds kicker")] = Anope::printf("%s (%d kick(s) to ban)", enabled.c_str(), kd->ttb[TTB_BOLDS]);
			else
				info[_("Bolds kicker")] = enabled;
		}
		else
			info[_("Bolds kicker")] = disabled;

		if (kd && kd->caps)
		{
			if (kd->ttb[TTB_CAPS])
				info[_("Caps kicker")] = Anope::printf(_("%s (%d kick(s) to ban; minimum %d/%d%%)"), enabled.c_str(), kd->ttb[TTB_CAPS], kd->capsmin, kd->capspercent);
			else
				info[_("Caps kicker")] = Anope::printf(_("%s (minimum %d/%d%%)"), enabled.c_str(), kd->capsmin, kd->capspercent);
		}
		else
			info[_("Caps kicker")] = disabled;

		if (kd && kd->colors)
		{
			if (kd->ttb[TTB_COLORS])
				info[_("Colors kicker")] = Anope::printf(_("%s (%d kick(s) to ban)"), enabled.c_str(), kd->ttb[TTB_COLORS]);
			else
				info[_("Colors kicker")] = enabled;
		}
		else
			info[_("Colors kicker")] = disabled;

		if (kd && kd->flood)
		{
			if (kd->ttb[TTB_FLOOD])
				info[_("Flood kicker")] = Anope::printf(_("%s (%d kick(s) to ban; %d lines in %ds)"), enabled.c_str(), kd->ttb[TTB_FLOOD], kd->floodlines, kd->floodsecs);
			else
				info[_("Flood kicker")] = Anope::printf(_("%s (%d lines in %ds)"), enabled.c_str(), kd->floodlines, kd->floodsecs);
		}
		else
			info[_("Flood kicker")] = disabled;

		if (kd && kd->repeat)
		{
			if (kd->ttb[TTB_REPEAT])
				info[_("Repeat kicker")] = Anope::printf(_("%s (%d kick(s) to ban; %d times)"), enabled.c_str(), kd->ttb[TTB_REPEAT], kd->repeattimes);
			else
				info[_("Repeat kicker")] = Anope::printf(_("%s (%d times)"), enabled.c_str(), kd->repeattimes);
		}
		else
			info[_("Repeat kicker")] = disabled;

		if (kd && kd->reverses)
		{
			if (kd->ttb[TTB_REVERSES])
				info[_("Reverses kicker")] = Anope::printf(_("%s (%d kick(s) to ban)"), enabled.c_str(), kd->ttb[TTB_REVERSES]);
			else
				info[_("Reverses kicker")] = enabled;
		}
		else
			info[_("Reverses kicker")] = disabled;

		if (kd && kd->underlines)
		{
			if (kd->ttb[TTB_UNDERLINES])
				info[_("Underlines kicker")] = Anope::printf(_("%s (%d kick(s) to ban)"), enabled.c_str(), kd->ttb[TTB_UNDERLINES]);
			else
				info[_("Underlines kicker")] = enabled;
		}
		else
			info[_("Underlines kicker")] = disabled;

		if (kd && kd->italics)
		{
			if (kd->ttb[TTB_ITALICS])
				info[_("Italics kicker")] = Anope::printf(_("%s (%d kick(s) to ban)"), enabled.c_str(), kd->ttb[TTB_ITALICS]);
			else
				info[_("Italics kicker")] = enabled;
		}
		else
			info[_("Italics kicker")] = disabled;

		if (kd && kd->amsgs)
		{
			if (kd->ttb[TTB_AMSGS])
				info[_("AMSG kicker")] = Anope::printf(_("%s (%d kick(s) to ban)"), enabled.c_str(), kd->ttb[TTB_AMSGS]);
			else
				info[_("AMSG kicker")] = enabled;
		}
		else
			info[_("AMSG kicker")] = disabled;

		if (kd && kd->dontkickops)
			info.AddOption(_("Ops protection"));
		if (kd && kd->dontkickvoices)
			info.AddOption(_("Voices protection"));
	}

	void OnPrivmsg(User *u, Channel *c, Anope::string &msg) override
	{
		/* Now we can make kicker stuff. We try to order the checks
		 * from the fastest one to the slowest one, since there's
		 * no need to process other kickers if a user is kicked before
		 * the last kicker check.
		 *
		 * But FIRST we check whether the user is protected in any
		 * way.
		 */
		ChanServ::Channel *ci = c->ci;
		if (ci == NULL)
			return;
		KickerData *kd = kickerdata.Get(ci);
		if (kd == NULL)
			return;

		if (ci->AccessFor(u).HasPriv("NOKICK"))
			return;
		else if (kd->dontkickops && (c->HasUserStatus(u, "HALFOP") || c->HasUserStatus(u, "OP") || c->HasUserStatus(u, "PROTECT") || c->HasUserStatus(u, "OWNER")))
			return;
		else if (kd->dontkickvoices && c->HasUserStatus(u, "VOICE"))
			return;

		Anope::string realbuf = msg;

		/* If it's a /me, cut the CTCP part because the ACTION will cause
		 * problems with the caps or badwords kicker
		 */
		if (realbuf.substr(0, 8).equals_ci("\1ACTION ") && realbuf[realbuf.length() - 1] == '\1')
		{
			realbuf.erase(0, 8);
			realbuf.erase(realbuf.length() - 1);
		}

		if (realbuf.empty())
			return;

		/* Bolds kicker */
		if (kd->bolds && realbuf.find(2) != Anope::string::npos)
		{
			check_ban(ci, u, kd, TTB_BOLDS);
			bot_kick(ci, u, _("Don't use bolds on this channel!"));
			return;
		}

		/* Color kicker */
		if (kd->colors && realbuf.find(3) != Anope::string::npos)
		{
			check_ban(ci, u, kd, TTB_COLORS);
			bot_kick(ci, u, _("Don't use colors on this channel!"));
			return;
		}

		/* Reverses kicker */
		if (kd->reverses && realbuf.find(22) != Anope::string::npos)
		{
			check_ban(ci, u, kd, TTB_REVERSES);
			bot_kick(ci, u, _("Don't use reverses on this channel!"));
			return;
		}

		/* Italics kicker */
		if (kd->italics && realbuf.find(29) != Anope::string::npos)
		{
			check_ban(ci, u, kd, TTB_ITALICS);
			bot_kick(ci, u, _("Don't use italics on this channel!"));
			return;
		}

		/* Underlines kicker */
		if (kd->underlines && realbuf.find(31) != Anope::string::npos)
		{
			check_ban(ci, u, kd, TTB_UNDERLINES);
			bot_kick(ci, u, _("Don't use underlines on this channel!"));
			return;
		}

		/* Caps kicker */
		if (kd->caps && realbuf.length() >= static_cast<unsigned>(kd->capsmin))
		{
			int i = 0, l = 0;

			for (unsigned j = 0, end = realbuf.length(); j < end; ++j)
			{
				if (isupper(realbuf[j]))
					++i;
				else if (islower(realbuf[j]))
					++l;
			}

			/* i counts uppercase chars, l counts lowercase chars. Only
			 * alphabetic chars (so islower || isupper) qualify for the
			 * percentage of caps to kick for; the rest is ignored. -GD
			 */

			if ((i || l) && i >= kd->capsmin && i * 100 / (i + l) >= kd->capspercent)
			{
				check_ban(ci, u, kd, TTB_CAPS);
				bot_kick(ci, u, _("Turn caps lock OFF!"));
				return;
			}
		}

		/* Bad words kicker */
		if (kd->badwords)
		{
			bool mustkick = false;
			BadWords *badwords = ci->GetExt<BadWords>("badwords");

			/* Normalize the buffer */
			Anope::string nbuf = Anope::NormalizeBuffer(realbuf);
			bool casesensitive = Config->GetModule("botserv")->Get<bool>("casesensitive");

			/* Normalize can return an empty string if this only conains control codes etc */
			if (badwords && !nbuf.empty())
				for (unsigned i = 0; i < badwords->GetBadWordCount(); ++i)
				{
					const BadWord *bw = badwords->GetBadWord(i);

					if (bw->word.empty())
						continue; // Shouldn't happen

					if (bw->word.length() > nbuf.length())
						continue; // This can't ever match

					if (bw->type == BW_ANY && ((casesensitive && nbuf.find(bw->word) != Anope::string::npos) || (!casesensitive && nbuf.find_ci(bw->word) != Anope::string::npos)))
						mustkick = true;
					else if (bw->type == BW_SINGLE)
					{
						size_t len = bw->word.length();

						if ((casesensitive && bw->word.equals_cs(nbuf)) || (!casesensitive && bw->word.equals_ci(nbuf)))
							mustkick = true;
						else if (nbuf.find(' ') == len && ((casesensitive && bw->word.equals_cs(nbuf.substr(0, len))) || (!casesensitive && bw->word.equals_ci(nbuf.substr(0, len)))))
							mustkick = true;
						else
						{
							if (len < nbuf.length() && nbuf.rfind(' ') == nbuf.length() - len - 1 && ((casesensitive && nbuf.find(bw->word) == nbuf.length() - len) || (!casesensitive && nbuf.find_ci(bw->word) == nbuf.length() - len)))
								mustkick = true;
							else
							{
								Anope::string wordbuf = " " + bw->word + " ";

								if ((casesensitive && nbuf.find(wordbuf) != Anope::string::npos) || (!casesensitive && nbuf.find_ci(wordbuf) != Anope::string::npos))
									mustkick = true;
							}
						}
					}
					else if (bw->type == BW_START)
					{
						size_t len = bw->word.length();

						if ((casesensitive && nbuf.substr(0, len).equals_cs(bw->word)) || (!casesensitive && nbuf.substr(0, len).equals_ci(bw->word)))
							mustkick = true;
						else
						{
							Anope::string wordbuf = " " + bw->word;

							if ((casesensitive && nbuf.find(wordbuf) != Anope::string::npos) || (!casesensitive && nbuf.find_ci(wordbuf) != Anope::string::npos))
								mustkick = true;
						}
					}
					else if (bw->type == BW_END)
					{
						size_t len = bw->word.length();

						if ((casesensitive && nbuf.substr(nbuf.length() - len).equals_cs(bw->word)) || (!casesensitive && nbuf.substr(nbuf.length() - len).equals_ci(bw->word)))
							mustkick = true;
						else
						{
							Anope::string wordbuf = bw->word + " ";

							if ((casesensitive && nbuf.find(wordbuf) != Anope::string::npos) || (!casesensitive && nbuf.find_ci(wordbuf) != Anope::string::npos))
								mustkick = true;
						}
					}

					if (mustkick)
					{
						check_ban(ci, u, kd, TTB_BADWORDS);
						if (Config->GetModule(me)->Get<bool>("gentlebadwordreason"))
							bot_kick(ci, u, _("Watch your language!"));
						else
							bot_kick(ci, u, _("Don't use the word \"%s\" on this channel!"), bw->word.c_str());

						return;
					}
				} /* for */
		} /* if badwords */

		UserData *ud = GetUserData(u, c);

		if (ud)
		{
			/* Flood kicker */
			if (kd->flood)
			{
				if (Anope::CurTime - ud->last_start > kd->floodsecs)
				{
					ud->last_start = Anope::CurTime;
					ud->lines = 0;
				}

				++ud->lines;
				if (ud->lines >= kd->floodlines)
				{
					check_ban(ci, u, kd, TTB_FLOOD);
					bot_kick(ci, u, _("Stop flooding!"));
					return;
				}
			}

			/* Repeat kicker */
			if (kd->repeat)
			{
				if (!ud->lastline.equals_ci(realbuf))
					ud->times = 0;
				else
					++ud->times;

				if (ud->times >= kd->repeattimes)
				{
					check_ban(ci, u, kd, TTB_REPEAT);
					bot_kick(ci, u, _("Stop repeating yourself!"));
					return;
				}
			}

			if (ud->lastline.equals_ci(realbuf) && !ud->lasttarget.empty() && !ud->lasttarget.equals_ci(ci->name))
			{
				for (User::ChanUserList::iterator it = u->chans.begin(); it != u->chans.end();)
				{
					Channel *chan = it->second->chan;
					++it;

					if (chan->ci && kd->amsgs && !chan->ci->AccessFor(u).HasPriv("NOKICK"))
					{
						check_ban(chan->ci, u, kd, TTB_AMSGS);
						bot_kick(chan->ci, u, _("Don't use AMSGs!"));
					}
				}
			}

			ud->lasttarget = ci->name;
			ud->lastline = realbuf;
		}
	}
};

MODULE_INIT(BSKick)

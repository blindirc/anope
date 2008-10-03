/* Bahamut functions
 *
 * (C) 2003-2008 Anope Team
 * Contact us at info@anope.org
 *
 * Please read COPYING and README for furhter details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 *
 */

/*************************************************************************/

#define UMODE_a 0x00000001  /* umode +a - Services Admin */
#define UMODE_h 0x00000002  /* umode +h - Helper */
#define UMODE_i 0x00000004  /* umode +i - Invisible */
#define UMODE_o 0x00000008  /* umode +o - Oper */
#define UMODE_r 0x00000010  /* umode +r - registered nick */
#define UMODE_w 0x00000020  /* umode +w - Get wallops */
#define UMODE_A 0x00000040  /* umode +A - Server Admin */
#define UMODE_x 0x00000080  /* umode +x - Squelch with notice */
#define UMODE_X 0x00000100  /* umode +X - Squelch without notice */
#define UMODE_F 0x00000200  /* umode +F - no cptr->since message rate throttle */
#define UMODE_j 0x00000400  /* umode +j - client rejection notices */
#define UMODE_K 0x00000800  /* umode +K - U: lined server kill messages */
#define UMODE_O 0x00001000  /* umode +O - Local Oper */
#define UMODE_s 0x00002000  /* umode +s - Server notices */
#define UMODE_c 0x00004000  /* umode +c - Client connections/exits */
#define UMODE_k 0x00008000  /* umode +k - Server kill messages */
#define UMODE_f 0x00010000  /* umode +f - Server flood messages */
#define UMODE_y 0x00020000  /* umode +y - Stats/links */
#define UMODE_d 0x00040000  /* umode +d - Debug info */
#define UMODE_g 0x00080000  /* umode +g - Globops */
#define UMODE_b 0x00100000  /* umode +b - Chatops */
#define UMODE_n 0x00200000  /* umode +n - Routing Notices */
#define UMODE_m 0x00400000  /* umode +m - spambot notices */
#define UMODE_e 0x00800000  /* umode +e - oper notices for the above +D */
#define UMODE_D 0x01000000  /* umode +D - Hidden dccallow umode */
#define UMODE_I 0x02000000  /* umode +I - invisible oper (masked) */
#define UMODE_R 0x80000000  /* unmode +R - No non registered msgs */

#define CMODE_i 0x00000001
#define CMODE_m 0x00000002
#define CMODE_n 0x00000004
#define CMODE_p 0x00000008
#define CMODE_s 0x00000010
#define CMODE_t 0x00000020
#define CMODE_k 0x00000040	/* These two used only by ChanServ */
#define CMODE_l 0x00000080
#define CMODE_R 0x00000100	/* Only identified users can join */
#define CMODE_r 0x00000200	/* Set for all registered channels */
#define CMODE_c 0x00000400	/* Colors can't be used */
#define CMODE_M 0x00000800      /* Non-regged nicks can't send messages */
#define CMODE_j 0x00001000      /* join throttle */
#define CMODE_O 0x00008000	/* Only opers can join */

#define DEFAULT_MLOCK CMODE_n | CMODE_t | CMODE_r

class BahamutIRCdProto : public IRCDProto {
		void SendSVSKillInternal(const char *, const char *, const char *);
		void SendModeInternal(BotInfo *, const char *, const char *);
		void SendKickInternal(BotInfo *, const char *, const char *, const char *);
		void SendNoticeChanopsInternal(BotInfo *, const char *, const char *);
	public:
		void SendSVSNOOP(const char *, int);
		void SendAkillDel(const char *, const char *);
		void SendTopic(BotInfo *, const char *, const char *, const char *, time_t);
		void SendAkill(const char *, const char *, const char *, time_t, time_t, const char *);
		void SendSVSMode(User *, int, const char **);
		void SendGuestNick(const char *, const char *, const char *, const char *, const char *);
		void SendClientIntroduction(const char *, const char *, const char *, const char *, const char *);
		void SendBotOp(const char *, const char *);
		void SendJoin(BotInfo *, const char *, time_t);
		void SendSQLineDel(const char *);
		void SendSQLine(const char *, const char *);
		void SendConnect();
		void SendSVSHold(const char *);
		void SendSVSHoldDel(const char *);
		void SendSGLineDel(const char *);
		void SendSZLineDel(const char *);
		void SendSZLine(const char *, const char *, const char *);
		void SendSGLine(const char *, const char *);
		void SendBanDel(const char *, const char *);
		void SendSVSModeChan(const char *, const char *, const char *);
		void SendSVID(const char *, time_t);
		void SendUnregisteredNick(User *);
		void SendSVID3(User *, const char *);
		void SendEOB();
		void SendServer(const char *, int, const char *);
		void ProcessUsermodes(User *, int, const char **);
		int IsFloodModeParamValid(const char *);
} ircd_proto;

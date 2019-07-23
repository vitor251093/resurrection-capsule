/// \file TeamBalancer.h
/// \brief Set and network team selection (supports peer to peer or client/server)
/// \details Automatically handles transmission and resolution of team selection, including team switching and balancing
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_TeamBalancer==1

#ifndef __TEAM_BALANCER_H
#define __TEAM_BALANCER_H

class RakPeerInterface;
#include "PluginInterface2.h"
#include "RakMemoryOverride.h"
#include "NativeTypes.h"
#include "DS_List.h"
#include "RakString.h"

namespace RakNet
{

/// \defgroup TEAM_BALANCER_GROUP TeamBalancer
/// \brief Set and network team selection (supports peer to peer or client/server)
/// \details Automatically handles transmission and resolution of team selection, including team switching and balancing
/// \ingroup PLUGINS_GROUP

/// 0...254 for your team number identifiers. 255 is reserved as undefined.
/// \ingroup TEAM_BALANCER_GROUP
typedef unsigned char TeamId;

#define UNASSIGNED_TEAM_ID 255

/// \brief Set and network team selection (supports peer to peer or client/server)
/// \details Automatically handles transmission and resolution of team selection, including team switching and balancing.<BR>
/// Usage: TODO
/// \ingroup TEAM_BALANCER_GROUP
class RAK_DLL_EXPORT TeamBalancer : public PluginInterface2
{
public:
	TeamBalancer();
	virtual ~TeamBalancer();

	/// \brief Define which system processes team communication and maintains the list of teams
	/// \details One system is responsible for maintaining the team list and determining which system is on which team.
	/// For a client/server system, this would be the server.
	/// For a peer to peer system, this would be one of the peers. If using FullyConnectedMesh2, this would be called with the value returned by FullyConnectedMesh2::GetHostSystem(). Update when you get ID_FCM2_INFORM_FCMGUID
	/// \sa SetAllowHostMigration()
	/// \param[in] _hostGuid One system we are connected to that will resolve team assignments.
	void SetHostGuid(RakNetGUID _hostGuid);

	/// \brief Set the limit to the number of players on each team
	/// \details SetTeamSizeLimits() must be called on the host, so the host can enforce the maximum number of players on each team.
	/// SetTeamSizeLimits() can be called on all systems if desired - for example, in a P2P environment you may wish to call it on all systems in advanced in case you become host.
	/// Calling this function when teams have already been created does not affect existing teams.
	/// \param[in] teamLimits The maximum number of people per team, by index. For example, a list of size 3 with values 1,2,3 would allow 1 person on team 0, 2 people on team 1, adn 3 people on team 2.
	void SetTeamSizeLimits(const DataStructures::List<unsigned short> &_teamLimits);

	/// \brief Set the limit to the number of players on each team
	/// \details SetTeamSizeLimits() must be called on the host, so the host can enforce the maximum number of players on each team.
	/// SetTeamSizeLimits() can be called on all systems if desired - for example, in a P2P environment you may wish to call it on all systems in advanced in case you become host.
	/// Calling this function when teams have already been created does not affect existing teams.
	/// \param[in] values The maximum number of people per team, by index. For example, a list of size 3 with values 1,2,3 would allow 1 person on team 0, 2 people on team 1, adn 3 people on team 2.
	/// \param[in] valuesLength Length of the values array
	void SetTeamSizeLimits(unsigned short *values, int valuesLength);
	
	enum DefaultAssigmentAlgorithm
	{
		/// Among all the teams, join the team with the smallest number of players
		SMALLEST_TEAM,
		/// Join the team with the lowest index that has open slots.
		FILL_IN_ORDER
	};
	/// \brief Determine how players' teams will be set when they call RequestAnyTeam()
	/// \details Based on the specified enumeration, a player will join a team automatically
	/// Defaults to SMALLEST_TEAM
	/// This function is only used by the host
	/// \param[in] daa Enumeration describing the algorithm to use
	void SetDefaultAssignmentAlgorithm(DefaultAssigmentAlgorithm daa);

	/// \brief By default, teams can be unbalanced up to the team size limit defined by SetTeamSizeLimits()
	/// \details If SetForceEvenTeams(true) is called on the host, then teams cannot be unbalanced by more than 1 player
	/// If teams are uneven at the time that SetForceEvenTeams(true) is called, players at randomly will be switched, and will be notified of ID_TEAM_BALANCER_TEAM_ASSIGNED
	/// If players disconnect from the host such that teams would not be even, and teams are not locked, then a player from the largest team is randomly moved to even the teams.
	/// Defaults to false
	/// \note SetLockTeams(true) takes priority over SetForceEvenTeams(), so if teams are currently locked, this function will have no effect until teams become unlocked.
	/// \param[in] force True to force even teams. False to allow teams to not be evenly matched
	void SetForceEvenTeams(bool force);

	/// \brief If set, calls to RequestSpecificTeam() and RequestAnyTeam() will return the team you are currently on.
	/// \details However, if those functions are called and you do not have a team, then you will be assigned to a default team according to SetDefaultAssignmentAlgorithm() and possibly SetForceEvenTeams(true)
	/// If \a lock is false, and SetForceEvenTeams() was called with \a force as true, and teams are currently uneven, they will be made even, and those players randomly moved will get ID_TEAM_BALANCER_TEAM_ASSIGNED
	/// Defaults to false
	/// \param[in] lock True to lock teams, false to unlock
	void SetLockTeams(bool lock);

	/// Set your requested team. UNASSIGNED_TEAM_ID means no team.
	/// After enough time for network communication, ID_TEAM_BALANCER_SET_TEAM will be returned with your current team, or
	/// If team switch is not possible, ID_TEAM_BALANCER_REQUESTED_TEAM_CHANGE_PENDING or ID_TEAM_BALANCER_TEAMS_LOCKED will be returned.
	/// In the case of ID_TEAM_BALANCER_REQUESTED_TEAM_CHANGE_PENDING the request will stay in memory. ID_TEAM_BALANCER_SET_TEAM will be returned when someone on the desired team leaves or wants to switch to your team.
	/// If SetLockTeams(true) is called while you have a request pending, you will get ID_TEAM_BALANCER_TEAMS_LOCKED
	/// \pre Call SetTeamSizeLimits() on the host and call SetHostGuid() on this system. If the host is not running the TeamBalancer plugin or did not have SetTeamSizeLimits() called, then you will not get any response.
	/// \param[in] desiredTeam An index representing your team number. The index should range from 0 to one less than the size of the list passed to SetTeamSizeLimits() on the host. You can also pass UNASSIGNED_TEAM_ID to not be on any team (such as if spectating)
	/// \return True on request sent, false on SetHostGuid() was not called.
	bool RequestSpecificTeam(TeamId desiredTeam);

	/// If ID_TEAM_BALANCER_REQUESTED_TEAM_CHANGE_PENDING is returned after a call to RequestSpecificTeam(), the request will stay in memory on the host and execute when available, or until the teams become locked.
	/// You can cancel the request by calling CancelRequestSpecificTeam(), in which case you will stay on your existing team.
	/// \note Due to latency, even after calling CancelRequestSpecificTeam() you may still get ID_TEAM_BALANCER_SET_TEAM if the packet was already in transmission.
	void CancelRequestSpecificTeam(void);

	/// Allow host to pick your team, based on whatever algorithm it uses for default team assignments.
	/// This only has an effect if you are not currently on a team (GetMyTeam() returns UNASSIGNED_TEAM_ID)
	/// \pre Call SetTeamSizeLimits() on the host and call SetHostGuid() on this system
	void RequestAnyTeam(void);

	/// Returns your team.
	/// As your team changes, you are notified through the ID_TEAM_BALANCER_TEAM_ASSIGNED packet in byte 1.
	/// Returns UNASSIGNED_TEAM_ID initially
	/// \pre For this to return anything other than UNASSIGNED_TEAM_ID, connect to a properly initialized host and RequestSpecificTeam() or RequestAnyTeam() first
	/// \return UNASSIGNED_TEAM_ID for no team. Otherwise, the index should range from 0 to one less than the size of the list passed to SetTeamSizeLimits() on the host
	TeamId GetMyTeam(void) const;

	/// Allow systems to change the host
	/// If true, this is a security hole, but needed for peer to peer
	/// For client server, set to false
	/// Defaults to true
	/// param[in] allow True to allow host migration, false to not allow
	void SetAllowHostMigration(bool allow);

	struct TeamMember
	{
		RakNetGUID memberGuid;
		TeamId currentTeam;
		TeamId requestedTeam;
	};

protected:

	/// \internal
	virtual PluginReceiveResult OnReceive(Packet *packet);
	/// \internal
	virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );

	void OnStatusUpdateToNewHost(Packet *packet);
	void OnCancelTeamRequest(Packet *packet);
	void OnRequestAnyTeam(Packet *packet);
	void OnRequestSpecificTeam(Packet *packet);

	RakNetGUID hostGuid;
	TeamId currentTeam;
	TeamId requestedTeam;
	DefaultAssigmentAlgorithm defaultAssigmentAlgorithm;
	bool forceTeamsToBeEven;
	bool lockTeams;
	bool expectingToReceiveTeamNumber; // So if we lose the connection while processing, we request the same info of the new host
	bool allowHostMigration;

	DataStructures::List<unsigned short> teamLimits;
	DataStructures::List<unsigned short> teamMemberCounts;
	DataStructures::List<TeamMember> teamMembers;
	unsigned int GetMemberIndex(RakNetGUID guid);
	unsigned int AddTeamMember(const TeamMember &tm); // Returns index of new member
	void RemoveTeamMember(unsigned int index);
	void EvenTeams(void);
	unsigned int GetMemberIndexToSwitchTeams(const DataStructures::List<TeamId> &sourceTeamNumbers, TeamId targetTeamNumber);
	void GetOverpopulatedTeams(DataStructures::List<TeamId> &overpopulatedTeams, int maxTeamSize);
	void SwitchMemberTeam(unsigned int teamMemberIndex, TeamId destinationTeam);
	void NotifyTeamAssigment(unsigned int teamMemberIndex);
	bool WeAreHost(void) const;
	PluginReceiveResult OnTeamAssigned(Packet *packet);
	PluginReceiveResult OnRequestedTeamChangePending(Packet *packet);
	PluginReceiveResult OnTeamsLocked(Packet *packet);
	void GetMinMaxTeamMembers(int &minMembersOnASingleTeam, int &maxMembersOnASingleTeam);
	TeamId GetNextDefaultTeam(void); // Accounting for team balancing and team limits, get the team a player should be placed on
	bool TeamWouldBeOverpopulatedOnAddition(TeamId teamId, unsigned int teamMemberSize); // Accounting for team balancing and team limits, would this team be overpopulated if a member was added to it?
	bool TeamWouldBeUnderpopulatedOnLeave(TeamId teamId, unsigned int teamMemberSize);
	TeamId GetSmallestNonFullTeam(void) const;
	TeamId GetFirstNonFullTeam(void) const;
	void MoveMemberThatWantsToJoinTeam(TeamId teamId);
	TeamId MoveMemberThatWantsToJoinTeamInternal(TeamId teamId);
	void NotifyTeamsLocked(RakNetGUID target, TeamId requestedTeam);
	void NotifyTeamSwitchPending(RakNetGUID target, TeamId requestedTeam);
	void NotifyNoTeam(RakNetGUID target);
	void SwapTeamMembersByRequest(unsigned int memberIndex1, unsigned int memberIndex2);
	void RemoveByGuid(RakNetGUID rakNetGUID);
	bool TeamsWouldBeEvenOnSwitch(TeamId t1, TeamId t2);

};

} // namespace RakNet

#endif

#endif // _RAKNET_SUPPORT_*

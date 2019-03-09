import Utils.BlazeFuncs as BlazeFuncs

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == '0000':
		print("[ASSLST] AddUserstoList")
	elif func == '00001':
		print("[ASSLST] RemoveUserstoList")
	elif func == '00002':
		print("[ASSLST] ClearLists")
	elif func == '00003':
		print("[ASSLST] SetUsersToList")
	elif func == '00004':
		print("[ASSLST] GetListForUs")
	elif func == '00005':
		print("[ASSLST] GetLists")
	elif func == '00006':
		print("[ASSLST] SubscribeToLis")
	elif func == '00007':
		print("[ASSLST] Unsubscribefro")
	elif func == '00008':
		print("[ASSLST] GetConfigLists")
	else:
		print("[ASSLST] ERROR! UNKNOWN FUNC "+func)
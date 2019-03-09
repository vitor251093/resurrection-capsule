# Darkspore.exe Addresses
Inside the game executable, there are the functions that make the game work. In order to make the game functional again, we have to understand what those functions are, and what they do. This is a list of the memory addresses that we already discover the purpose, and is also a reference for future findings. We have given provisory names to those functions and variables in order to make them more understandable from a developer perspective.

All the notes taken here consider only the latest official version of the game (5.3.0.127). Parameters named has `u*` have unknown purposes.

## Known functions

### sub_401A80: allocStringWithSize
**Full name:** `void allocStringWithSize(void* stringPointer, unsigned int size)``
**Example:** `allocStringWithSize(&string, 0x11u);`
**Purpose:** Allocs in `stringPointer` a string with size `size`. No content is written in that space.

### sub_402380: allocAndInitString
**Full name:** `void allocAndInitString(void* stringPointer, void* string);`
**Example:** `allocAndInitString(&string, "Version");`
**Purpose:** Allocs in `stringPointer` a string with `string`'s size +1, and writes `string` in that space.

### sub_45D810: getXmlValueForKey
**Full name:** `int getXmlValueForKey(void* xml, int key_begin_address, int u1, int u2, int u3)`
**Example:** `getXmlValueForKey(xml, key_begin_address, ?, ?, ?);`
**Purpose:** Returns the value of the mentioned key from the `xml` dictionary/map.

### sub_45E310: retrieveAccountDetailsFromXmlObj
**Full name:** `unsigned __int64 retrieveAccountDetailsFromXmlObj(void* account, int u1)`
**Example:** `retrieveAccountDetailsFromXmlObj(account, ?);`
**Purpose:** Read the account details from an `account` object retrieved from a xml dictionary/map.

### sub_461600: getSurveysCallback
**Full name:** `void getSurveysCallback(int u1, void* xml)`
**Example:** `getSurveysCallback(?, xml)`
**Purpose:** The callback of the getSurveys function, with the returned `xml` dictionary/map.

### sub_461EC0: bootstrapApiCallback
**Full name:** `char bootstrapApiCallback(int u1, void** u2, int u3, int u4)`
**Example:** `bootstrapApiCallback(?, ?, ?, ?)`
**Purpose:** The first step of all bootstrap/api functions responses.

### sub_468120: retrieveBroadcastsDetailsFromXmlObj
**Full name:** `void retrieveBroadcastsDetailsFromXmlObj(int response, _DWORD* broadcasts)`
**Example:** `retrieveBroadcastsDetailsFromXmlObj(response, broadcasts)`
**Purpose:** Read the broadcasts details from a `broadcasts` object retrieved from a xml dictionary/map, and store them in `response`.

### sub_469CE0: getBroadcastsCallback
**Full name:** `void getBroadcastsCallback(_DWORD* u1, void* xml)`
**Example:** `getBroadcastsCallback(?, xml);`
**Purpose:** The callback of the getBroadcasts function, with the returned `xml` dictionary/map.

### sub_469E00: getStatusCallback
**Full name:** `void getStatusCallback(int u1, void* xml)`
**Example:** `getStatusCallback(?, xml);`
**Purpose:** The callback of the getStatus function, with the returned `xml` dictionary/map.

### sub_46BDD0: getConfigsCallback
**Full name:** `void getConfigsCallback(void* configs, void* xml)`
**Example:** `getConfigsCallback(configs, xml);`
**Purpose:** The callback of the getConfigs function, with the returned `xml` dictionary/map, and store them in `configs`.

### sub_AE44B0: stringToInteger
**Full name:** `unsigned __int64 stringToInteger(signed __int16* string, int* u1, signed int u2)`
**Example:** `stringToInteger(string, ?, ?);`
**Purpose:** Parse `string` into an unsigned integer number.


## Unknown functions

### Possibly connected with an event handler
- sub_4015B0
- sub_4015F0
- sub_551F10

### Possibly connected with the Blaze callback
- sub_C2D2B0
- sub_C2D370
- sub_C2A8D0
- sub_DEFED0
- sub_DF0520
- sub_DEC400
- sub_DEFED0
- sub_DF0520
- sub_E3E9A0
- sub_E415D0
- sub_E3E9A0
- sub_DEEA60
- sub_DEC1E0
- sub_DEDAC0
- sub_DEFE70
- sub_DF0050
- sub_C2A500
- sub_53AB80
- sub_5371F0
- sub_5373B0

### Possibly connected with the Blaze callback (login screen error handler)
- sub_E07760

## Known errors

### BlazeLoginManager errors
- sub_C465B0: Blaze login failed
- sub_C46610: Login SDK Error
- sub_C468D0: Error getting Nucleus Auth Token
- sub_C469B0: Login persona failed
- sub_C46A10: Create Persona Error
- sub_C46B00: Prepare for create account failed
- sub_C46B50: Create account failed

## Known strings
- off_FDB54C: "gms"
- off_FDB788: "false"

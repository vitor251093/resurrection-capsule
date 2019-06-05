# Darkspore.exe EAWebKit Addresses
This is the list of known EAWebKit functions inside the Darkspore.exe. We are trying to find out how the game works to make the server, and for that, the more we know about the game code, the best.

## ProtoSSL
- **sub_E4DB30:** \_ProtoSSLUpdateRecvServerCert **IMPORTANT**
- **sub_E4DC00:** \_ProtoSSLUpdateSendClientKey
- **sub_E4DEC0:** \_ProtoSSLUpdateSendChange
- **sub_E4DF10:** \_ProtoSSLUpdateSendClientFinish
- **sub_E4E0E0:** \_ProtoSSLUpdateRecvChange
- **sub_E4E110:** \_ProtoSSLUpdateRecvServerFinish

## Important
If we can use Detours to make `sub_E4DB30` always return `21`, it will never refuse a received package.

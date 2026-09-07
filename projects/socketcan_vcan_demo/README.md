# SocketCAN vcan Demo

This project covers week 8 with CAN frame basics, SocketCAN command vocabulary, vcan loopback, actuator status decoding, periodic control commands, and runtime fault mapping. It does not require a real CAN device; the C++ programs simulate the important contracts with deterministic output.

## Verify

```bash
cd projects/socketcan_vcan_demo
bash scripts/verify_socketcan_vcan_demo.sh
```

Expected output:

```txt
[socketcan] basic command checklist
[can_frame] raw=123#1122334455667788 id=0x123 dlc=8 data="11 22 33 44 55 66 77 88"
[can_frame] raw=321#AABBCCDD id=0x321 dlc=4 data="AA BB CC DD"
[ok] CAN frame basics verified
[ok] vcan loopback command path verified
[ok] actuator status receiver decoded position velocity and fault
[ok] periodic command sender generated 50Hz control frames
[ok] CAN timeout and actuator fault map to runtime Fault
```

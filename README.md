# IrpProxy
This is a tool for analyzing and fuzzing Windows drivers. 

It can:
1. Hook the IRP handling routines of a target driver to record all IRPs it receives.
2. Mutate and replay the recorded communications. 

# Code organization

* IrpProxy is the kernel driver that replaces the IRP handling routines of the target driver to hook all communications sent to the driver
* IrpProxyClient is a usermode application that interacts with IrpProxy
* IrpReplay is a usermode application that mutates and replays the communications recorded by IrpProxyClient

# How to use
1. Install the kernel driver:

sc create IrpProxy type= kernel binPath= c:\path\to\IrpProxy.sys

2. Hook the target driver's dispatch routines: 

IrpProxyClient.exe /hook \<Target Device Name\>

3. Write the recorded IRPs to a file:

IrpProxyClient.exe /getData

4. Mutate and replay the recorded IRPs:

IrpReplay.exe \<FileName.dat\> \<Target Device Name\>

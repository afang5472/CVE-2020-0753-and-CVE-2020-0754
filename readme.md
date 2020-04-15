### Writeup and POC for CVE-2020-0753, CVE-2020-0754 and six fixed Window DOS Vulnerabilities

#### Exploit FileSystem race condition bug - analysis of CVE-2020-0753 and CVE-2020-0754

Windows Error Reporting service has fixed 2 Elevation of Privilege bugs in last Patch Tuesday, the two bugs are assigned with [CVE-2020-0753](https://portal.msrc.microsoft.com/en-us/security-guidance/advisory/CVE-2020-0753) and [CVE-2020-0754](https://portal.msrc.microsoft.com/en-us/security-guidance/advisory/CVE-2020-0754). Both of the two bugs leverage a race condition bug in the service's FileSystem operations.  However, those two bugs are not so easy to exploit due to small race windows and uncertain file drop locations. Here we share our techniques to exploit them. 

The root cause of the two race bugs are given in our reports, the actual cause can be expressed as **Predictable is Vulnerable**. When WER service processes temp files, it manipulates file location `C:\ProgramData\Microsoft\Windows\WER\Temp`, which is a R/W enabled directory for Authenticated Users. That means a medium-IL normal user can overwrite a file created by WER service, and even turn it into a FileSystem link to damage/delete other files they could have been unable to touch. 

To keep file operations safe, WER service relies on a standard api named `GetTempFileNameW` and wrapped it with `wersvc.dll->UtilGetTempFile`, this api will help WerSvc to generate unoccupied random file name in the form of `"WER****.tmp"`,

the random part of the file name is generated with a 4-byte hex number, from `0000-FFFF`, if one number has been taken to create a file, the api will take another random file name.

The strategy clearly has a flaw if one creates 65535 files naming from `WER0000.tmp` to 

`WERFFFE.tmp`, the api will pick a random number and test the filename to see if it exists, like `WERA560.tmp`, it will find the file already exists, thus it will continue to test from `WERA560.tmp` until `WERFFFF.tmp`, during the test goes on, a preparing window appears because we have found a way making WerSvc get stuck on the `GetTempFileNameW` call for 4-5 seconds, which is pretty a big timing gap. In the meantime, we force the service to drop a temporary file with fixed file name, that is 

`WERFFFF.tmp`. 

After the service creates the tempfile named `WERFFFF.tmp`, the api automatically 

closes the handle it holds to the file, and returns the file name to the service for further operations on the file, this is right the position introducing the bug. Three conditions are met:

1. File Created by service is at normal user controllable position.

2. Service closes all handles to the file

3. Service will use the file later(write or delete)

Here the service will **write contents into the file and delete it.** Both write and delete will cause privilege escalation by leveraging FileSystem links and certain exploitation techniques. 

In order to turn the bug into arbitrary file deletion, we creatively leverage Multiple directory junctions to finish exploiting. Our exploit contains the following steps:

- we put all the `WER***.tmp` in `$pwd\1\`, and junction `$pwd\2\ -> $pwd\1\`;
- we create a process to continuously trigger this function with the path `$pwd\2\`. and create the other process to continuously execute the command `SetOplock $pwd\1\WERFFFF.tmp`;
- Once the Oplock is triggered , we junction `$pwd\2\ -> \RPC CONTROL\`, and then create object symbolic `\RPC CONTROL\WERFFFF.tmp -> $target` and `\RPC CONTROL\WERFFFF.tmp.etl -> $target`
- Release the oplock , the target file will be deleted with system privilege.

The detailed exploitation and poc are provided in WERReport-CVE-2020-0753.

By exploiting the flaw in `GetTempFileNameW`, we gain a predictable location the service will operate; by making use of multiple level FileSystem junction, we make race condition reliably exploitable. 

In the meanwhile, we noticed that this kind of race bug can also cause possible file overwriting issue, then it's likely leading to Escalation of Privilege bugs under certain circumstance.

#### From arbitrary file corruption with partial control to privilege escalation

To explain the reason why arbitrary file corruption (if you can control a very small part of the file content: less than 63 bytes) can be turned into EoP, we need to pay attention to Windows Defender's working mechanism.

Windows Defender has a malware signature database. If some file contain some malware signature, defender will consider it's a malware and delete them. However, this feature will result in additional attack surface. For exmaple, In  [WCTF2019](https://ctf.360.com/index_en.html) @icchy from tokyowesterns design a Windows ctf challenge name  ["Gyotaku The Flag"](https://westerns.tokyo/wctf2019-gtf/wctf2019-gtf-slides.pdf), which use this feature as a oracle to leak information. 

Here we leverage this feature of Windows Defender to delete arbitrary file if we have a  arbitrary file corruption with partial control of the file content.  We can just write a  malware signature into a file and triggering a default scan by Windows Defender, the file will be put into isolation area of Defender, which can be deleted by a normal user(i.e. a medium-IL non-admin user) simply by triggering the scan operation for 2 times.

So an arbitrary file corruption bug can be turned into arbitrary file deletion as long as a malware signature string can be put into the target file using the bug.

- Step1:

Corrupt the target file with a bug, put a feature string recognizable by Windows Defender into it.

- Step2:

Trigger Windows Defender to scan the target file, causing the file isolated.

- Step3:

Trigger the scan again, target file deleted.

By leveraging the technique we acquire an arbitrary file deletion with Defender's help.

Arbitrary file deletion can be exploited much easier to gain further privilege.

#### Six fixed FileSystem DOS vulnerabilities in Microsoft OneDrive

Microsoft OneDrive is the application bundle providing Personal Cloud Storage service,

this application has been integrated with Windows as a default installation option since Windows 8. During our research, 6 vulnerabilities in OneDrive's servicing scheduled tasks were found and submitted to MSRC.

---

Here's a table of vulnerabilities we gonna expose in Microsoft OneDrive relevant scheduled tasks:

| Vulnerable Program            | Type     | POC Provided |
| ----------------------------- | -------- | ------------ |
| FileSyncConfig.exe            | HardLink | yes          |
| FileSyncHelper.exe            | HardLink | yes          |
| OneDriveFileSyncConfig.exe    | SymLink  | yes          |
| OneDriveSetup.exe             | HardLink | yes          |
| OneDriveSetup.exe             | HardLink | yes          |
| OneDriveStandaloneUpdater.exe | HardLink | yes          |

All the 6 bugs are caused by the service improperly handles hardlinks and symlinks, while operating locations that normal user is controllable. During the exploitation of these bugs, a difficulty comes from the filename usually contains a pid of the current process or a timestamps marking when the file is operated. Both can be solved by setting an oplock on a unique dll file that the service will try to load when it is triggered to run, where we have the ability to get everything we need to predict the filename the service will try to operate later. An example POC is provided in FileSyncConfigTemp_hardlink directory.

#### Vulnerability Impacts

All 6 vulnerabilities stated above are provided with complete report and poc program, although most bugs are causing arbitrary file corruption in the first place, this kind of bug still causing System crash(By overwriting critical system configuration file), and all of them would require reinstallation of Windows. Thus meeting the standard of Windows System Denial of Service bug type. 

Besides, this kind of bug can actually cause Elevation of Privilege under certain context. We have discussed the exploiting technique that can leverage an arbitrary file overwrite problem to achieve arbitrary file deletion primitive, thus Elevation of Privilege is achievable. 

#### Vulnerability Credits

[Fangming Gu](https://twitter.com/afang5472)

[Zhiniang Peng of Qihoo 360 Core Security](https://twitter.com/edwardzpeng)

#### Time Line

**Feb 02 2020:** Vulnerabilities reported

**Feb 08 2020:**  MSRC investigated and replied about the 6 bugs in OneDrive we submitted, their conclusion is not fix because of **Too much user interaction required/too difficult to build a reliable exploit**. 

**Feb 08 2020:** We replied: There is no need for user interaction. You just need to wait for the task schedule to run.  So, this scenario is typical. 

**Feb 11 2020:** MSRC replied: How do you get the specific file on the users machine? And are you placing every permutation of that file in that folder? Does it have to match Date/Hour/PID exactly. Its for these reasons that this seems like it takes too much user effort. 

**Feb 11 2020:** We replied: Our poc is a simplified version. To reduce the effort of predict the filename. In reality, You just need to set a oplock. Then you can get all of the {pid},{hour},{data}. So, no need for user interaction. 

**Feb 12 2020:** Asking can we publish the writeup for those 6 vulnerabilities. 

**Feb 13 2020:** MSRC replied:  You may post a writeup.

**Feb 22 2020:** Details published

Status Upgrade: All 6 vulnerabilities has received a fix in March 2020's patch Tuesday.



---
name: '"X" app doesn''t work with VxKex!'
about: Use this template when an app is not functioning correctly with VxKex enabled.
title: "[Bug Report] *ProgramName v1.x* doesn't work on Windows 7 using VxKex"
labels: bug
assignees: ''

---

### Application Information

- **Application**: [Insert name and Version]
- **Download Link**: [Provide direct link to download]

### Description

A brief overview of the problem you're encountering, including any specific details about the issue.

### Problem

1. Step-by-step instructions to reproduce the bug/error.
2. Attach **screenshots** or **error messages** related to the issue.

### Logs

To help developers investigate the issue, please follow the steps below to collect and share the logs:

1. **Clear existing logs**:
   - Navigate to `%LOCALAPPDATA%\Local\VxKex\Logs`
   - Delete all the log files in this folder.

2. **Reproduce the issue**:
   - Launch the application with **VxKex enabled**.
   - Reproduce the error or behavior you are reporting.

3. **Collect logs**:
   - Close the application (or close the error window).
   - Go back to `%LOCALAPPDATA%\Local\VxKex\Logs` and **zip** all newly created log files.

4. **Attach the zipped log file here.**

### Optional: Collect Additional Logs with YY-Thunks  

To provide more detailed information about APIs used by the application, you can use **YY-Thunks**:  

1. Download **YY-Thunks** from the official release page (https://github.com/Chuyu-Team/YY-Thunks/releases)
2. Run the following command in a terminal, replacing `D:\Tool\SomeProgram.exe` with the path to your application:  
   ```  
   YY.Depends.Analyzer.exe "D:\Tool\SomeProgram.exe" /IgnoreReady /ReportView:Table /Target:6.1.7600  
   ```  
3. This will generate a Markdown file named `SomeProgram.exe.md` in the same directory as the analyzer. The file contains a list of API/s that the application might use and is available only on Windows 8 or higher.  

4. Open the `.md` file, copy its entire contents, and paste it into the collapsible section below.

<details>
  <summary>Click here to see YY-Thunks report</summary>

PASTE THE ENTIRE .MD CONTENT HERE

</details>

### Environment Details

Please provide the following details about your environment:

- **Operating System**: (e.g., Windows 7 SP1 English, 64bit, with ESU updates)
- **VxKex Version**: (e.g., 1.1.2.1428; if using a custom fork, provide the fork repository link)

Feel free to add any other details or context that might be helpful for reproducing the issue or understanding the behavior.

# Samecopy

This is a very basic and lightweight wrapper for RoboCopy.exe written
(mostly) in plain Win32.

What is RoboCopy?

For those unaware, Windows ships with a program called Robocopy.exe
AKA "Robust File Copy", which among other things, allows you to copy the
contents of one folder to another while preserving all the metadata. Rather
unfortunately, it is only a command line tool however, so using it is a bit
inconvenient. You can read more here about [Robocopy] here if you want. 


Thus, Samecopy was born.
There are existing projects that accomplish what Samecopy does, but they rely
on .NET framework and I dislike needless dependencies for small programs.

##### Disclaimer
This is an unfinished project with known issues. Use at your own risk.

### Usage
  - Run the program
 
  - Use the "..." buttons to select the source and destination folders
    or just drag and drop the folders onto the text boxes.

  - Click copy

  - RoboCopy is launched in a seperate console window (Do not close it) and displays
    the current progress of the copy operation.

  - When complete, the console window closes and you will be notified via 
    a message box which also gives you the option to explore the 
    destination folder.

### Issues
- Unsupported characters: Certain unicode characters are not supported.
- Paths are limited to MAX_PATH (260 characters)
- No logging: The program tells you to "check the log" when something goes wrong, 
  but there is no log; It is an artifact of the time when I redirected the console output to a text file.
- The code is full of integer and string literals. A refactoring is needed.


[Robocopy]: <https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/robocopy?redirectedfrom=MSD>

Running cache-efi.c with no OS:

## SETTING UP THE BUILD ENVIRONMENT ##

MAC OS X:

Follow the instructions on [EFI
Programming](http://www.osxbook.com/book/bonus/chapter4/efiprogramming/)
found in the Mac OS X Book website.


## RUNNING THE CODE ##

Download refit from

http://refit.sourceforge.net/

- Get a USB keychain or similar
- Launch Disk Utility
  * make 2 partitions, one HFS+ (small, say 50MB) and the other
    FAT32. This is because EFI can't write to HFS+ file systems, so we
    need a FAT32 FS to get the results.
  * select "Restore" from rEFIt-0.13.dmg to USB stick's HFS+ FS
  * copy cache.efi to /Volumes/EFI/efi/tools
  * eject the USB stick
- Reboot, hold Opt key
- Boot from rEFIt
- Select "Start EFI Shell"
- Let startup.nsh run
- Find the FAT32 partition, in my case, it was in fs3.
<pre>
    Shell> fs3:
    fs3:\> cache 268435456 2> EFI_OUT
    ...
</pre>
Note that it's *very important* to redirect stderr to a
file. Otherwise, the efi shell might hang (something about not being
able to multiplex)

- Reboot again, now you have the output in the FAT32 partition
- Since the resulting file is UTF-16 encoded, you need to convert it
  to UTF-8 so that gnuplot can read it.
<pre>
    $ iconv -f UTF-16 -t UTF-8 EFI_OUT > EFI_OUT_UTF8
</pre>
- Now you can plot it.  

  
### HOW TO USE EFI Shell ###

[http://www.icare.hp.com.cn/TechCenter_StaticArticle/11503/13416.pdf]()


# fix-my-holiday-mp4

Some code to literally fix a single broken MP4 video that I took while on holiday many years ago. The phone died after recording a significant amount of video, and didn't get a chance to write the header to the file. MP4 files can have their headers at the beginning or the end, but the end is most convenient for recording devices, as it will then know all of the interesting metadata. Unfortunately that also means that without those headers, the `mdat` box is just some unknown video and audio samples. The playing device doesn't know how to decode them.

Basically a hand-written parser for a small subset of MP4 box types that I've found in the broken video file, and working video files from the same device that would allow me to splice working metadata onto the truncated video.

Getting a hold of the MP4 spec can be difficult (and I always forget where I found it last) so here's a hopefully working link: <https://web.archive.org/web/20180219054429/http://l.web.umkc.edu/lizhu/teaching/2016sp.video-communication/ref/mp4.pdf>

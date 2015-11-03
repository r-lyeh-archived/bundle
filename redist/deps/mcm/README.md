mcm compressor: context mixing + lzp
===================================

###### Fast + effective CM algorithm
<p>
- LZP for faster compression of highly compressible files
<p>
- Binary + text detection for model switching
<p>
- E8E9 filter for X86 binaries

## Benchmarks:
Sandy Bridge 2630QM, 8GB RAM, Windows 7 x64, GCC compile (make.bat)

<p>
- Silesia corpus (separate files) from http://mattmahoney.net/dc/silesia.html:
<table>
<tr><th>Program</th><th>Compression size</th><th>Compression time
<tr><th>Uncompressed</th><th>211,938,580</th><th>N/A
<tr><th>7z ultra 1 thread</th><th>48,745,801</th><th>140s
<tr><th>CCM 7</th><th>44,954,217</th><th>70s
<tr><th>CCMX 7</th><th>43,970,142</th><th>86s
<tr><th>mcm v0.82 -turbo -9</th><th>42,837,008</th><th>81s
<tr><th>mcm v0.82 -fast -9</th><th>41,819,611</th><th>91s
<tr><th>mcm v0.82 -mid -9</th><th>40,541,614</th><th>116s
<tr><th>mcm v0.82 -high -9</th><th>40,242,195</th><th>132s
<tr><th>mcm v0.82 -max -9</th><th>40,068,926</th><th>140s
</table>

<p>
- ENWIK8 from http://mattmahoney.net/dc/text.html:
<table>
<tr><th>Program</th><th>Compression size</th><th>Compression time</th><th>Decompression time
<tr><th>Uncompressed</th><th>100,000,000</th><th>N/A</th><th>N/A
<tr><th>CCM 7</th><th>21,980,533</th><th>35s</th><th>Unmeasured
<tr><th>CCMX 7</th><th>20,857,925</th><th>46s</th><th>Unmeasured
<tr><th>mcm v0.82 -turbo -9</th><th>20,429,116</th><th>48s</th><th>Unmeasured
<tr><th>mcm v0.83 -t9</th><th>20,199,979</th><th>40s</th><th>34s
<tr><th>mcm v0.82 -fast -9</th><th>19,958,144</th><th>51s</th><th>Unmeasured
<tr><th>zcm 0.92 -m7 -t1 </th><th>19,803,554</th><th>45s</th><th>Unmeasured
<tr><th>mcm v0.82 -mid -9</th><th>19,520,204</th><th>60s</th><th>Unmeasured
<tr><th>mcm v0.82 -high -9</th><th>19,318,179</th><th>73s</th><th>Unmeasured
<tr><th>mcm v0.83 -f9</th><th>19,313,858</th><th>43s</th><th>36s
<tr><th>mcm v0.82 -max -9</th><th>19,211,781</th><th>80s</th><th>Unmeasured
<tr><th>mcm v0.83 -m9</th><th>18,627,061</th><th>49s</th><th>44s
<tr><th>mcm v0.83 -h9</th><th>18,504,139</th><th>58s</th><th>54s
<tr><th>mcm v0.83 -x9</th><th>18,379,121</th><th>64s</th><th>59s
</table>

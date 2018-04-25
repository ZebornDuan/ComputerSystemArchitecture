import os
import sys
import glob
import gzip

result = open('result.out', 'r')
t = result.readlines()
result.close()

g = open('readme.md', 'w')
g.write('| **Program** | **LRU_time** | **LRU_CPI** | **LRU_miss** | \
**Random_time** | **Random_CPI** | **DRRIP_miss** | \
**DRRIP_time** | **DRRIP_CPI** | **DRRIP_miss** |\n\
| ----: | -------: | ----: | ----: | \
-------: | ----: | ----: | \
-------: | ----: | ----: |\n')

for (i, file) in enumerate(glob.glob(os.getcwd().replace('\\', '/') + '/runs/*')):
	with gzip.open(file, 'r') as f:
		if i % 3 == 0:
			g.write('| ' + file.split('.')[1][:-1])
		lines = f.read().decode('utf-8').split('\n')
		g.write(' | ' + t[3 * i].replace('\n', '').split(' ')[-1])
		try:
			g.write(' | ' + lines[-44].split(' ')[-7])
		except:
			g.write(' | ' + lines[-39].split(' ')[-7])
		g.write(' | ' + lines[-6].split(' ')[-1])
		# print(i)
		# print(t[3 * i].replace('\n', '').split(' ')[-1])
		# print(lines[-6].split(' ')[-1])
		# print(lines[-44].split(' ')[-7])
		if i % 3 == 2:
			g.write(' |\n')


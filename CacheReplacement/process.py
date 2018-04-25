import os
import sys
import glob
import gzip

result = open('result.out', 'r')
t = result.readlines()
result.close()

g = open('readme.md', 'w')
g.write('| **Program** | **LRU_time** | **LRU_CPI** | **LRU_miss** | \
**Random_time** | **Random_CPI** | **Random_miss** | \
**DRRIP_time** | **DRRIP_CPI** | **DRRIP_miss** | \
**least_time** | **least_CPI** | **least_miss** |\n\
| ----: | -------: | ----: | ----: | \
-------: | ----: | ----: | \
-------: | ----: | ----: | \
-------: | ----: | ----: |\n')

algorithm = ['LRU', 'Random', 'DRRIP']
m3 = [0] * 3
c3 = [0] * 3
for (i, file) in enumerate(glob.glob(os.getcwd().replace('\\', '/') + '/runs/*')):
	with gzip.open(file, 'r') as f:
		if i % 3 == 0:
			g.write('| ' + file.split('.')[1][:-1])
		lines = f.read().decode('utf-8').split('\n')
		g.write(' | ' + t[3 * i].replace('\n', '').split(' ')[-1])
		try:
			cpi = lines[-44].split(' ')[-7]
		except:
			cpi = lines[-39].split(' ')[-7]
		g.write(' | ' + cpi)
		c3[i % 3] = float(cpi)
		miss = lines[-6].split(' ')[-1]
		g.write(' | ' + miss)
		m3[i % 3] = float(miss)
		if i % 3 == 2:
			t3 = [float(t[3 * (i - 2 + j) ].replace('\n', '').split(' ')[-1]) for j in range(3)]
			print(t3)
			g.write(' | ' + algorithm[t3.index(min(t3[0], t3[1], t3[2]))])
			if not m3[0] == m3[1] == m3[2]:
				g.write(' | ' + algorithm[c3.index(min(c3[0], t3[1], c3[2]))])
			else:
				g.write(' | --')
			if not m3[0] == m3[1] == m3[2]:
				g.write(' | ' + algorithm[m3.index(min(m3[0], m3[1], m3[2]))])
			else:
				g.write(' | --')
			g.write(' |\n')


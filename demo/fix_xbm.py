#!/usr/bin/python -u
# vim: fileencoding=utf-8:nomodified:nowrap:textwidth=0:foldmethod=marker:foldcolumn=4:ruler:showcmd:lcs=tab\:|- list:tabstop=8:noexpandtab:nosmarttab:softtabstop=0:shiftwidth=0

import re
import sys

def upravit_xbm_soubor(nazev_souboru):
	name=nazev_souboru[:nazev_souboru.find('.')]
	with open(nazev_souboru, 'r') as soubor:
		obsah = soubor.readlines()

	# Najít šířku obrazu
	width = None
	for radek in obsah:
		match = re.match(r'#define\s+(\w+)_width\s+(\d+)', radek)
		if match:
			width = int(match.group(2))
		match = re.match(r'#define\s+(\w+)_height\s+(\d+)', radek)
		if match:
			height = int(match.group(2))

	if width is None:
		print("Chyba: Šířka obrazu nebyla nalezena.")
		return

	print(f"width {width}")
	# Počet hodnot na řádek (byte = 8 bitů)
	hodnot_na_radek = (width+7) // 8
	print(f"hodnot_na_radek {hodnot_na_radek}")
	
	# Upravit datové sekce
	nova_data = []
	hodnoty = []
	v_datoch = False
	nova_data.append(f'#include "{name}.h"\n\n')
	for radek in obsah:
		match = re.match(r'#define\s+', radek)
		if match:
			radek = f"//	{radek}"
		if 'static unsigned char' in radek and '{' in radek:
			v_datoch = True
			nova_data.append(f"//	{radek}\n")
			nova_data.append(f"const uint8_t  __attribute__ ((aligned(256))) {name}[{height}][{width//8}] PROGMEM =\n")
			nova_data.append("	{\n");
			nova_data.append("	\n");
		elif v_datoch:
			# Pokud je to data, převeď hodnoty na nové řádky
			radek = radek.strip().strip(',')
			if radek:
				for x in radek.split(','):
					hodnoty.append(x.strip().strip(',').replace("}","\n}"))
			if '};' in radek:
				v_datoch = False
				for i in range(0, len(hodnoty), hodnot_na_radek):
					nova_data.append('	' + ', '.join(hodnoty[i:i+hodnot_na_radek]) + ',\n')
				nova_data[len(nova_data)-1]=nova_data[len(nova_data)-1].rstrip().rstrip(',')
		else:
			nova_data.append(radek)

	# Uložit upravený soubor
	with open('fix_' + nazev_souboru, 'w') as soubor:
		soubor.writelines(nova_data)

	print(f"{nazev_souboru} -> fix_{nazev_souboru}")

# Příklad použití
if ( len(sys.argv) != 2):
	print("pouziti: fix_xbm.py soubor.xbm")
	sys.exit(1)
upravit_xbm_soubor(sys.argv[1])


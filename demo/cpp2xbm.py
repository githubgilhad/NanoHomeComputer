#!/usr/bin/python -u
# vim: fileencoding=utf-8:nomodified:nowrap:textwidth=0:foldmethod=marker:foldcolumn=4:ruler:showcmd:lcs=tab\:|- list:tabstop=8:noexpandtab:nosmarttab:softtabstop=0:shiftwidth=0

import re
import sys

def upravit_cpp_soubor(nazev_souboru):
	with open(nazev_souboru, 'r') as soubor:
		obsah = soubor.readlines()
	novy_nazev_souboru=f"{nazev_souboru[:nazev_souboru.find('.')]}.xbm"

	# Najít šířku obrazu
	width = None
	posice = 0
	for radek in obsah:
		posice += 1
		match = re.match(r'.*\s([A-Za-z_][A-Za-z0-9_]*)\s*\[(\d+)]\[(\d+)]', radek)
		if match:
			varname = match.group(1)
			height = int(match.group(2))
			width = int(match.group(3))*8
			break

	if width is None:
		print("Chyba: Šířka obrazu nebyla nalezena.  hledam neco jako charset_had[9][256]")
		return

	print(f"{height}x{width}")
	# Počet hodnot na řádek (byte = 8 bitů)
	hodnot_na_radek = 12
	print(f"hodnot_na_radek {hodnot_na_radek}")
	
	# Upravit datové sekce
	nova_data = []
	hodnoty = []
	v_datech = False
	for radek in obsah:
		posice -= 1
		if (posice>=0):
			continue
		if '{' in radek:
			v_datech = True
			radek = radek[radek.find('{')+1:]
		if v_datech:
			# Pokud je to data, převeď hodnoty na nové řádky
			radek = radek.strip().strip(',')
			if radek:
				for x in radek.split(','):
					hodnoty.append(x.strip().strip(',').replace("}","\n}"))
			if '};' in radek:
				v_datech = False
				for i in range(0, len(hodnoty)-1, hodnot_na_radek):
					nova_data.append('   ' + ', '.join(hodnoty[i:i+hodnot_na_radek]) + ',\n')
				nova_data[len(nova_data)-1]=nova_data[len(nova_data)-1].rstrip().rstrip(',')
		else:
			nova_data.append(radek)

	with open(novy_nazev_souboru, 'w') as soubor:
		soubor.write(f"#define {varname}_width {width}\n")
		soubor.write(f"#define {varname}_height {height}\n")
		soubor.write(f"static unsigned char {varname}_bits[] = "+"{\n")
		soubor.writelines(nova_data)
		soubor.write("\n};\n")

	print(f"{nazev_souboru} -> {novy_nazev_souboru}")

# Příklad použití
if ( len(sys.argv) != 2):
	print("pouziti: cpp2xbm.py soubor.cpp")
	print("	upravi soubor.cpp na soubor.xbm, nebo se o to aspon pokusi")
	sys.exit(1)
upravit_cpp_soubor(sys.argv[1])


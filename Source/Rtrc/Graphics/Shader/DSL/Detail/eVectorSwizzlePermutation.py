# Usage:
#	python.exe .\eVectorSwizzlePermutation.py >> eVectorSwizzlePermutation.txt

def process(component_names):

	for input_component in range(2, 5):
		print(f"#ifdef VECTOR_SWIZZLE_PERMUTATION_{input_component}_2")
		for x in range(input_component):
			for y in range(input_component):
				print(f"	VECTOR_SWIZZLE_PERMUTATION_{input_component}_2({x}, {y}, {component_names[x]}{component_names[y]})")
		print("#endif")
		
		print(f"#ifdef VECTOR_SWIZZLE_PERMUTATION_{input_component}_3")
		for x in range(input_component):
			for y in range(input_component):
				for z in range(input_component):
					print(f"	VECTOR_SWIZZLE_PERMUTATION_{input_component}_3({x}, {y}, {z}, {component_names[x]}{component_names[y]}{component_names[z]})")
		print("#endif")

		print(f"#ifdef VECTOR_SWIZZLE_PERMUTATION_{input_component}_4")
		for x in range(input_component):
			for y in range(input_component):
				for z in range(input_component):
					for w in range(input_component):
						print(f"	VECTOR_SWIZZLE_PERMUTATION_{input_component}_4({x}, {y}, {z}, {w}, {component_names[x]}{component_names[y]}{component_names[z]}{component_names[w]})")			
		print("#endif")

process(["x", "y", "z", "w"])
process(["r", "g", "b", "a"])

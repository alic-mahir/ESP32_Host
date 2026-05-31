
import os
import re

def process_file(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()

    new_lines = []
    inside_function = False
    brace_count = 0

    for line in lines:
        # Detect function start
        if re.match(r'^\s*\w+\s+\w+\s*\(', line):
            inside_function = True
            brace_count = 0
        if inside_function:
            if '{' in line:
                brace_count += line.count('{')
                new_lines.append(line)
                if brace_count == 1 and '{' in line:  # Add ifdef after the opening brace
                    new_lines.append('#ifdef PORT\n')
            elif '}' in line:
                brace_count -= line.count('}')
                if brace_count == 0 and inside_function:  # End of function
                    new_lines.append('#endif\n')
                    inside_function = False
                new_lines.append(line)
            else:
                new_lines.append(line)


            
        else:
            new_lines.append(line)

    # Write back the modified content
    with open(file_path, 'w') as file:
        file.writelines(new_lines)

def main():
    directory = '02_drivers'
    for filename in os.listdir(directory):
        if filename.endswith('.c'):
            process_file(os.path.join(directory, filename))

if __name__ == "__main__":
    main()


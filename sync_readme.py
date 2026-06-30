import os
import re

def fix_links_and_images(content, folder_path):
    pattern = r'(!?)\[([^\]]+)\]\(([^)]+)\)'
    def replace_match(match):
        is_img = match.group(1)
        text = match.group(2)
        path = match.group(3)
        if path.startswith(('http://', 'https://', '#', '/')):
            return f'{is_img}[{text}]({path})'
        combined_path = os.path.normpath(os.path.join(folder_path, path))
        normalized_path = combined_path.replace(os.sep, '/')
        return f'{is_img}[{text}](/{normalized_path})'
    return re.sub(pattern, replace_match, content)

def main():
    template_file = '.github-readme-template.md'
    if not os.path.exists(template_file):
        print('ERROR: Target file .github-readme-template.md was not located.')
        exit(1)
        
    with open(template_file, 'r', encoding='utf-8') as f:
        template = f.read()

    linux_readme_path = 'linux/README.md'
    if os.path.exists(linux_readme_path):
        with open(linux_readme_path, 'r', encoding='utf-8') as f:
            linux_raw_content = f.read()
        processed_content = fix_links_and_images(linux_raw_content, 'linux')
        doc_block = f'<details open>\n  <summary><b>📂 System Documentation: LINUX</b></summary>\n  \n  {processed_content}\n  \n</details>'
    else:
        doc_block = '> *Warning: linux/README.md file was not found during execution.*'

    output_content = template.replace('<!-- AUTO_GENERATED_DOCS -->', doc_block)

    with open('README.md', 'w', encoding='utf-8') as f:
        f.write(output_content)
    print('README.md combined and optimized successfully!')

if __name__ == '__main__':
    main()

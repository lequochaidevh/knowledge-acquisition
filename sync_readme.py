import os
import re

def fix_backwork_document_paths(content):
    # 1. Convert all HTML tags containing src="../" or src="../../../" back to src="./"
    # Example: src="../document/img.png" -> src="./document/img.png"
    content = re.sub(r'src=["\'](?:\.\./)+(document/[^"\']+)["\']', r'src="./\1"', content)
    
    # 2. Fix all Markdown links ending with ]( that contain multi-level parent directory paths like ../, ../../..
    # This flattens all nested traversal paths to point directly from the root directory level `./`
    # Example: guide](../third-party/guide.md) -> guide](./third-party/guide.md)
    # Example: [Doc](../../../../document/guide.md) -> [Doc](./document/guide.md)
    content = re.sub(r'(\]\]?\()(?:\.\./)+', r'\1./', content)
    
    return content

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
            
        # Execute the absolute regex path replacement here
        processed_content = fix_backwork_document_paths(linux_raw_content)
        
        doc_block = f'<details open>\n  <summary><b> 📂 System Documentation: LINUX</b></summary>\n  \n  {processed_content}\n  \n</details>'
    else:
        doc_block = '> *Warning: linux/README.md file was not found during execution.*'

    output_content = template.replace('<!-- AUTO_GENERATED_DOCS -->', doc_block)

    with open('README.md', 'w', encoding='utf-8') as f:
        f.write(output_content)
    print('README.md combined and document paths resolved successfully!')

if __name__ == '__main__':
    main()


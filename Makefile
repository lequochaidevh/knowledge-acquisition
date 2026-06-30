.PHONY: readme
readme:
	# Append the main header template to the root README
	@cat __HEADER_README > README.md
	# Append the section title and a horizontal separator
	@echo "# Linux Integration" >> README.md
	@echo "\`\`\`sh" >> README.md
	@echo "cd linux/" >> README.md
	@echo "\`\`\`" >> README.md
	@echo "---" >> README.md
	# Append an auto-generation warning comment
	@echo "\n<!-- Auto-generated - Do not edit directly -->\n" >> README.md
	
	# Directly replace '../' with './' inside your markdown file
	@cat linux/README.md | sed 's|\.\./|\./|g' >> README.md
	
	# Print a success message upon completion
	@echo "Main README successfully generated and asset paths fixed for GitHub!"

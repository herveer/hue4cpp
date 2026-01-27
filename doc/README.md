# Documentation

This directory contains documentation for the hue4cpp project.

## Doxygen Documentation

The project uses Doxygen to generate API documentation from the source code.

### Generating Documentation Locally

To generate the documentation locally, run:

```bash
doxygen Doxyfile
```

The generated HTML documentation will be available in `doc/generated/html/`. Open `doc/generated/html/index.html` in your browser to view it.

### Automated Documentation Generation

Documentation can be automatically generated via the GitHub Actions workflow:

1. Go to the "Actions" tab in the GitHub repository
2. Select "Generate Doxygen Documentation" from the workflows list
3. Click "Run workflow"
4. Select the source branch you want to generate documentation from
5. Click "Run workflow"

The workflow will:
- Generate documentation from the selected branch
- Create a new branch named `<source-branch>-autodoc`
- Commit the generated documentation to the new branch
- Open a pull request back to the source branch

### Generated Documentation

The generated documentation includes:
- API documentation for all public headers
- Class hierarchies and relationships
- Function and method documentation
- Source code browsing
- File and namespace documentation

### Documentation Directory Structure

```
doc/
├── generated/          # Auto-generated Doxygen output (gitignored)
│   └── html/          # HTML documentation
└── hueApiV2/          # Philips Hue API reference materials
```

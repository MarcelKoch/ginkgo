# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html
import os
import subprocess
from pathlib import Path

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

# Disable doxygen for now
# import subprocess, os
#
# # Doxygen
# subprocess.call('doxygen Doxyfile.in', shell=True)

project = 'Ginkgo'
copyright = '2024, The Ginkgo Authors'
author = 'The Ginkgo Authors'
release = '1.9.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'myst_parser',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.autosectionlabel',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.mathjax',
    'sphinx.ext.ifconfig',
    'sphinx.ext.viewcode',
    'sphinx_sitemap',
    'sphinx.ext.inheritance_diagram',
    'sphinxcontrib.doxylink',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

highlight_language = 'c++'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_theme_options = {
    'canonical_url': '',
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'logo_only': False,
    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}
html_static_path = ['_static']
html_logo = '../assets/logo_doc.png'
html_title = f'{project} v{release}'

html_baseurl = "https://greole.github.io/ginkgo"


# -- MyST configuration -------------------------------------------------------
# https://myst-parser.readthedocs.io/en/latest/syntax/optional.html

myst_enable_extensions = [
    "amsmath",
    "colon_fence",
    "deflist",
    "dollarmath",
    "linkify",
    "replacements",
    "smartquotes"
]

# -- Setup Doxygen ----------------------------------------------------------


read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

if read_the_docs_build:
    ginkgo_root = (Path(__file__) / "..").resolve()
    ginkgo_include = (ginkgo_root / "include").resolve()
    build_dir = (ginkgo_root / "build").resolve()

    subprocess.run(['cmake', '--build', build_dir, '-t', 'usr'])
else:
    pass


# -- doxylink configuration -------------------------------------------------
# https://sphinxcontrib-doxylink.readthedocs.io/en/stable/#

if read_the_docs_build:
    doxylink = {
        'gko': (f'{build_dir}/doc/_doxygen/Ginkgo.tag', f'{build_dir}/doc/_doxygen/usr')
    }
else:
    doxylink = {
        'gko': ('_doxygen/Ginkgo.tag', '../_doxygen/usr')
    }

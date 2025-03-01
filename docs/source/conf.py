import subprocess

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Hush Engine'
copyright = '2025, Alan Ramirez & Leonidas Gonzalez'
author = 'Alan Ramirez & Leonidas Gonzalez'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
	'breathe'
]

templates_path = ['_templates']
exclude_patterns = []


# html_sidebars = { '**': ['globaltoc.html'] }
# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
# html_static_path = ['_static']

html_theme_options = {
	# Toc options
	'collapse_navigation': True,
	'sticky_navigation': True,
	'navigation_depth': 4,
	'includehidden': True,
	'titles_only': False
}

breathe_projects = {
	"Hush Engine": "../doxybuild/xml/"
}
breathe_default_project = "Hush Engine"
breathe_default_members = ('members', 'undoc-members')
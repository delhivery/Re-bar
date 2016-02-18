'''
Installer for pyexpresso.manager
'''

from setuptools import setup, find_packages

setup(
    name='pyexpresso',
    description=(
        'Python utilities to call and parse expected path from package scans'),
    version='1.0.0',
    packages=find_packages(),
    author='Amit Prakash Ambasta',
    author_email='amit.ambasta@delhivery.com',
    classifiers=[
        'Development Status :: 5 - Stable',
        'Intended Audience :: lm-tech@delhivery.com',
        'Topic :: Data Services :: Path Tools',
        'Programming Language :: Python :: 2.7',
    ],
    keywords='express routing expected_path',
    zip_safe=True
)

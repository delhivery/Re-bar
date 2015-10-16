'''
Creates tarballs for installation of rebar
'''

from setuptools import setup, find_packages


setup(
    name='rebar',
    description=(
        'A python package to create and update Expected Paths '
        'given scan information against packages via Kinesis'),
    version='1.0.0',
    packages=find_packages(),

    author='Amit Prakash Ambasta',
    author_email='amit.ambasta@delhivery.com',

    install_requires=[
        'pymongo',
        'amazon_kclpy',
        'pydisque',
        'graphviz',
    ],
    classifiers=[
        'Development Status :: 5 - Stable',
        'Intended Audience :: lm-tech@delhivery.com',
        'Topic :: Data Services :: Path Tools',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.4',
    ],
    keywords='express routing expected_path',
)

Job system
===================

Hush Engine includes a threadpool which serves as a foundation for its job system. The job
system allows to schedule and execute tasks concurrently, making it easier to write
multithreaded code without having to manage threads directly.

.. contents::
    :local:

Introduction
------------------
.. _Introduction:
.. index:: Introduction

The job system is the base for most of the multithreaded operations of Hush Engine.

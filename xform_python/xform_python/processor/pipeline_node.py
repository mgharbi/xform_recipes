# ----------------------------------------------------------------------------
# File:    pipeline_node.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
# Abstract base class of pipeline nodes
#
# ---------------------------------------------------------------------------#


from abc import ABCMeta, abstractmethod

class PipelineNode(object):
    __metaclass__ = ABCMeta

    def setParams(self,p):
        self.p = p

    @abstractmethod
    def process(self, model):
        pass

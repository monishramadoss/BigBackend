
class DataSet(object):
    def __init__(self):
        pass

    def getitem(self, idx):
        return None

    def __getitem__(self, idx): 
        data = self.getitem(idx)           
        return data
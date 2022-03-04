

class DataLoader(object):
    def __init__(self, dataset):
        self.dataset = dataset

    def __getitem__(self, idx):
        tensor = self.dataset[idx]    
        return tensor

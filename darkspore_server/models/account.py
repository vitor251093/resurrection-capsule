class Account(object):

    def __init__(self, id, email, name):
        self.id = id       # Important note: 'id' and 'blaze_id' (or 'blazeid') are the same
        self.email = email
        self.name = name
        self.level = 0
        self.planetLevel = 0
        self.planetSublevel = 0
        self.tutorialCompleted = False

    def chainProgression(self):
        # Planet level and sublevel (bitlevel information; #0110 => Planet level 1, Sublevel 3)
        if self.level == 0:
            return 0
        return self.planetLevel*4 + (self.planetSublevel - 1)
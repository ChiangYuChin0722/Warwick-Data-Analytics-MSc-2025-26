import random
"""This game plays hangman with the user."""

class Hangman:

    def __init__(self):
        self.hidden_word = self.find_word()
        self.blank_string = "-" * len(self.hidden_word)
        self.lives = 6
        self.guessed_letters = [] # Keep track of guessed letters
        
        #For debugging only ;)
        print(f"Hidden word (for debugging): {self.hidden_word}")

    def process_guess(self, guess):
        """
        Processes a valid, new guess from the user.
        Updates self.blank_string if the guess is correct.
        Decrements self.lives if the guess is incorrect.
        """
        if guess in self.hidden_word:
            print(f"Good guess! '{guess}' is in the word.")
            
            # Rebuild the blank_string with the new letter
            # We have to do this because strings are immutable
            new_blanks = list(self.blank_string)
            for i in range(len(self.hidden_word)):
                if self.hidden_word[i] == guess:
                    new_blanks[i] = guess
            self.blank_string = "".join(new_blanks)
            
        else:
            print(f"Sorry, '{guess}' is not in the word.")
            self.lives -= 1 # Lose a life
        
    def find_word(self):
        # This method is complete
        # Use a try-except block for file handling, as /usr/share/dict/words
        # may not exist on all systems (e.g., Windows).
        try:
            with open('/usr/share/dict/words','r') as dictionary:
                words = dictionary.readlines() # Read all lines into a list
        except FileNotFoundError:
            print("Warning: Dictionary file '/usr/share/dict/words' not found.")
            print("Using a default fallback word list.")
            words = ['python', 'hangman', 'computer', 'programming', 'developer', 'world']
        
        return random.choice(words).lower().strip()
        
    def draw_hangman(self, lives):
        if lives == 6:
            print("=========\n ||     |\n ||\n ||\n ||\n ||\n/  \\")
        elif lives == 5: 
            print("=========\n ||     |\n ||     O\n ||\n ||\n ||\n/  \\")
        elif lives == 4:
            print("=========\n ||     |\n ||     O\n ||     |\n ||\n ||\n/  \\")
        elif lives == 3:
            print("=========\n ||     |\n ||    \O\n ||     |\n ||\n ||\n/  \\")
        elif lives == 2: 
            print("=========\n ||     |\n ||    \O/\n ||     |\n ||\n ||\n/  \\")
        elif lives == 1: 
            print("=========\n ||     |\n ||    \O/\n ||     |\n ||    /\n ||\n/  \\")
        elif lives == 0:
            print("=========\n ||     |\n ||     O \n ||    /|\\\n ||    / \\\n ||\n/  \\")
            
    def won_game(self):
        """
        Checks the win condition.
        Returns True if the user has won, False otherwise.
        """
        # The game is won if the blank string matches the hidden word
        # (i.e., there are no more dashes '-')
        return self.blank_string == self.hidden_word
        
    def play(self):
       """
       Runs the main game loop.
       Handles user input, validation, and win/loss conditions.
       """
       print("Welcome to Hangman!")
       
       # The game loop continues as long as the user has lives
       # AND has not yet won the game.
       while self.lives > 0 and not self.won_game():
            # 1. Display the current game state
            self.draw_hangman(self.lives)
            print(f"\nWord to guess: {self.blank_string}")
            print(f"Lives remaining: {self.lives}")
            print(f"Guessed letters: {' '.join(self.guessed_letters)}")

            # 2. Get and validate user input
            guess = input("Guess a letter: ").lower().strip() # Use input() for Python 3
            
            # Check for invalid input (not a single letter)
            if not guess.isalpha() or len(guess) != 1:
                print("Invalid input. Please enter a single letter.")
                print("----------------------------------------")
                continue # Skip the rest of the loop and ask again
                
            # Check if letter was already guessed
            if guess in self.guessed_letters:
                print(f"You already guessed '{guess}'. Try again.")
                print("----------------------------------------")
                continue # Skip the rest of the loop and ask again
                
            # 3. Process the valid, new guess
            self.guessed_letters.append(guess)
            self.process_guess(guess)
            print("----------------------------------------")

       # 4. Post-game: Check why the loop ended (win or loss)
       if self.won_game():
           print("\nCongratulations! You won!")
           print(f"The word was: {self.hidden_word}")
       else:
           # This means self.lives reached 0
           print("\nSorry, you ran out of lives.")
           self.draw_hangman(0) # Show the final hangman drawing
           print(f"The word was: {self.hidden_word}")
        
    
if __name__ == "__main__":
    game = Hangman()
    game.play()
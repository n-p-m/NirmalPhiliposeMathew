import os
import openai
import gradio as gr

#if you have OpenAI API key as an environment variable, enable the below
#openai.api_key = os.getenv("OPENAI_API_KEY")

#if you have OpenAI API key as a string, enable the below
openai.api_key = "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"

prompt = "The following is a conversation with an AI assistant. The assistant will provide recommendations related to child support.\n\nCaregiver: Hello, I think my child is having a rough time?\nAI:Can you tell me more about it??\nCaregiver: "

def openai_create(prompt):

    response = openai.Completion.create(
    model="davinci:ft-personal-2023-04-23-01-13-37",
    prompt=prompt,
    temperature=0.7,
    max_tokens=150,
    top_p=1,
    frequency_penalty=0,
    presence_penalty=0.6,
    stop=[" Caregiver:", " AI:"]
    )

    return response.choices[0].text



def chatgpt_clone(input, history):
    history = history or []
    s = list(sum(history, ()))
    s.append(input)
    inp = ' '.join(s)
    output = openai_create(inp)
    history.append((input, output))
    return history, history


block = gr.Blocks()


with block:
    gr.Markdown("""<h1><center>Chatbot</center></h1>
    """)
    chatbot = gr.Chatbot()
    message = gr.Textbox(placeholder=prompt)
    state = gr.State()
    submit = gr.Button("SEND")
    submit.click(chatgpt_clone, inputs=[message, state], outputs=[chatbot, state])

block.launch(debug = True)

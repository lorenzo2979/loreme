# Architecture draft only: this does not yet package native Speech-to-Phrase
# and Whisper into one tested runtime. DO NOT deploy to production.
FROM python:3.11-slim
WORKDIR /app
COPY agent.py .
CMD ["python", "-c", "from agent import judge; print('Experimental judge ready; STT backends not connected')"]

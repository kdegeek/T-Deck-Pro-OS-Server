FROM python:3.11-slim

WORKDIR /app

# Install system dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements and install Python dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY server.py .
COPY mqtt_client.py .

# Create data directory
RUN mkdir -p data/ota-updates data/logs static

# Expose port
EXPOSE 8000

# Run the server
CMD ["python", "server.py"]